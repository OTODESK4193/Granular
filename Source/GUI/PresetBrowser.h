// ==========================================
// File: PresetBrowser.h
// プリセットブラウザ（Factory 150 + User + Favorites）。Mainタブ下の領域に展開。
//   - 3カラム階層フィルタリング:
//       1. メインカテゴリ (All / Favorites ★ / 各ファクトリカテゴリ / User)
//       2. サブカテゴリ   (All / 選択カテゴリに応じたサブカテゴリ一覧)
//       3. プリセット一覧 (★/☆、プリセット名、Category / Sub タグ)
//   - 上部: 検索窓（全フィールド横断）、▲▼、RANDOM、SAVE（ダイアログ）、INIT、CLOSE
//   - キーボード ↑↓ でプリセット切替
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <vector>

#include "ColorPalette.h"
#include "../Presets/PresetData.h"

class PresetBrowser : public juce::Component
{
public:
    std::function<void(int)>                                                           onLoadFactory;
    std::function<void(const juce::File&)>                                             onLoadUser;
    std::function<void(const juce::String&)>                                           onSave;
    std::function<void(const juce::String&, const juce::String&, const juce::String&)> onSaveWithMeta;
    std::function<void()>                                                              onInit;
    std::function<void()>                                                              onRandom;
    std::function<void()>                                                              onClose;

    explicit PresetBrowser(juce::File presetDir) : userDir(presetDir)
    {
        setWantsKeyboardFocus(true);
        loadFavorites();

        categories.add("All");
        categories.add(juce::String::fromUTF8(u8"Favorites ★"));
        for (const auto& p : GranularPresets::all())
            if (!categories.contains(p.category)) categories.add(p.category);
        categories.add("User");

        catList.setModel(&catModel);
        catList.setRowHeight(24);
        catList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        catList.setWantsKeyboardFocus(false);
        addAndMakeVisible(catList);

        subCatList.setModel(&subCatModel);
        subCatList.setRowHeight(24);
        subCatList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        subCatList.setWantsKeyboardFocus(false);
        addAndMakeVisible(subCatList);

        presetList.setModel(&presetModel);
        presetList.setRowHeight(24);
        presetList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        presetList.setWantsKeyboardFocus(false);
        addAndMakeVisible(presetList);

        search.setTextToShowWhenEmpty("Search presets, categories...", GranularColors::textDim);
        search.setColour(juce::TextEditor::backgroundColourId, GranularColors::knobTrack);
        search.setColour(juce::TextEditor::textColourId, GranularColors::text);
        search.onTextChange = [this] { rebuild(); };
        addAndMakeVisible(search);

        auto styleBtn = [this](juce::TextButton& b, juce::Colour c)
        {
            b.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
            b.setColour(juce::TextButton::textColourOffId, c);
            addAndMakeVisible(b);
        };
        styleBtn(prevBtn, GranularColors::text);
        styleBtn(nextBtn, GranularColors::text);
        styleBtn(randBtn, GranularColors::textDim);
        styleBtn(saveBtn, GranularColors::mint);
        styleBtn(initBtn, GranularColors::text);
        styleBtn(closeBtn, GranularColors::pink);

        prevBtn.onClick   = [this] { step(-1); };
        nextBtn.onClick   = [this] { step(+1); };
        randBtn.onClick   = [this] { if (onRandom) onRandom(); else randomizePreset(); };
        saveBtn.onClick   = [this] { showSaveDialog(); };
        initBtn.onClick   = [this] { if (onInit) onInit(); };
        closeBtn.onClick  = [this] { if (onClose) onClose(); };

        updateSubCategories();
        rebuild();
    }

    void refresh()
    {
        updateSubCategories();
        rebuild();
    }

    void visibilityChanged() override
    {
        if (isVisible())
        {
            grabKeyboardFocus();
            refresh();
        }
    }

    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::upKey)   { step(-1); return true; }
        if (k == juce::KeyPress::downKey) { step(+1); return true; }
        return false;
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GranularColors::bg);
        g.setColour(GranularColors::panelLine);
        g.drawRect(getLocalBounds(), 1);

        // タイトル
        g.setColour(GranularColors::lavender);
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText("PRESETS", 12, 6, 80, 18, juce::Justification::centredLeft);

        // カラム見出しバー (y: 38, h: 16)
        auto r = getLocalBounds().reduced(8);
        r.removeFromTop(30); // top header
        auto headerRow = r.removeFromTop(16);

        auto catH = headerRow.removeFromLeft(160);
        headerRow.removeFromLeft(6);
        auto subH = headerRow.removeFromLeft(150);
        headerRow.removeFromLeft(6);
        auto preH = headerRow;

        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.setColour(GranularColors::lavender.withAlpha(0.65f));
        g.drawText("CATEGORY", catH.reduced(4, 0), juce::Justification::centredLeft);
        g.drawText("SUBCATEGORY", subH.reduced(4, 0), juce::Justification::centredLeft);
        g.drawText("PRESET NAME", preH.reduced(4, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8);
        auto top = r.removeFromTop(30);
        top.removeFromLeft(80); // "PRESETS" title
        search.setBounds(top.removeFromLeft(220).reduced(2));

        // 右側ボタン
        closeBtn.setBounds(top.removeFromRight(64).reduced(2));
        initBtn.setBounds(top.removeFromRight(56).reduced(2));
        saveBtn.setBounds(top.removeFromRight(56).reduced(2));
        nextBtn.setBounds(top.removeFromRight(28).reduced(2));
        prevBtn.setBounds(top.removeFromRight(28).reduced(2));
        randBtn.setBounds(top.removeFromRight(68).reduced(2));

        r.removeFromTop(18); // カラム見出し分
        catList.setBounds(r.removeFromLeft(160));
        r.removeFromLeft(6);
        subCatList.setBounds(r.removeFromLeft(150));
        r.removeFromLeft(6);
        presetList.setBounds(r);
    }

    void showSaveDialog()
    {
        auto* win = new juce::AlertWindow("Save User Preset",
                                          "Select Category / Subcategory and enter Preset Name.",
                                          juce::MessageBoxIconType::NoIcon,
                                          this);
        juce::StringArray catOptions;
        for (const auto& c : categories)
        {
            if (c != "All" && c != juce::String::fromUTF8(u8"Favorites ★"))
                catOptions.add(c);
        }
        if (!catOptions.contains("User")) catOptions.add("User");

        win->addComboBox("category", catOptions, "Category");
        if (auto* cb = win->getComboBoxComponent("category"))
        {
            cb->setEditableText(true);
            int idx = catOptions.indexOf(lastSavedCat);
            cb->setSelectedId(idx >= 0 ? idx + 1 : 1);
        }
        win->addTextEditor("subcategory", lastSavedSub, "Subcategory");
        win->addTextEditor("name", lastSavedName, "Preset Name");
        win->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        win->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        juce::Component::SafePointer<PresetBrowser> safeThis(this);
        win->enterModalState(true, juce::ModalCallbackFunction::create(
            [safeThis, win](int result)
            {
                std::unique_ptr<juce::AlertWindow> owned(win);
                if (result != 1 || safeThis == nullptr) return;

                juce::String cat = "User";
                if (auto* cb = owned->getComboBoxComponent("category"))
                {
                    const auto t = cb->getText().trim();
                    if (t.isNotEmpty()) cat = t;
                }
                const auto sub = owned->getTextEditorContents("subcategory").trim();
                const auto name = owned->getTextEditorContents("name").trim();

                if (name.isNotEmpty())
                {
                    safeThis->lastSavedCat = cat;
                    safeThis->lastSavedSub = sub;
                    safeThis->lastSavedName = name;
                    if (safeThis->onSaveWithMeta)
                        safeThis->onSaveWithMeta(name, cat, sub);
                    else if (safeThis->onSave)
                        safeThis->onSave(name);

                    safeThis->updateSubCategories();
                    safeThis->rebuild();
                }
            }), false);
    }

private:
    struct Entry { bool isFactory; int factoryIndex; juce::File file; juce::String name, cat, sub; };
    struct UserMeta { juce::File file; juce::String name, cat, sub; };

    juce::File userDir;
    juce::StringArray categories, subCategories, favorites;
    int selCat = 0;
    int selSubCat = 0;
    int currentRow = -1;
    std::vector<Entry> entries;

    juce::String lastSavedCat { "User" }, lastSavedSub, lastSavedName;

    juce::ListBox catList, subCatList, presetList;
    juce::TextEditor search;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, randBtn { "RANDOM" },
                     saveBtn { "SAVE" }, initBtn { "INIT" }, closeBtn { "CLOSE" };

    juce::File favFile() const { return userDir.getChildFile("_favorites.txt"); }
    void loadFavorites()
    {
        favorites.clear();
        if (favFile().existsAsFile())
            favorites.addLines(favFile().loadFileAsString());
        favorites.removeEmptyStrings();
    }
    void saveFavorites() { favFile().replaceWithText(favorites.joinIntoString("\n")); }
    void toggleFavorite(const juce::String& name)
    {
        if (favorites.contains(name)) favorites.removeString(name);
        else favorites.add(name);
        saveFavorites();
        rebuild();
    }

    std::vector<UserMeta> getUserMetas()
    {
        std::vector<UserMeta> metas;
        if (!userDir.isDirectory()) return metas;
        for (auto& f : userDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
        {
            juce::String nm = f.getFileNameWithoutExtension();
            juce::String c = "User";
            juce::String s = "";
            std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(f));
            if (xml != nullptr)
            {
                nm = xml->getStringAttribute("PresetName", nm);
                c  = xml->getStringAttribute("Category", "User");
                s  = xml->getStringAttribute("Subcategory", "");
            }
            metas.push_back({ f, nm, c, s });
        }
        return metas;
    }

    void updateSubCategories()
    {
        subCategories.clear();
        subCategories.add("All");

        if (selCat < 0 || selCat >= categories.size()) selCat = 0;
        const juce::String cat = categories[selCat];
        const bool favView = (cat == juce::String::fromUTF8(u8"Favorites ★"));
        auto userMetas = getUserMetas();

        juce::StringArray subs;
        if (cat != "User")
        {
            for (const auto& p : GranularPresets::all())
            {
                if (cat != "All" && !favView && p.category != cat) continue;
                if (favView && !favorites.contains(p.name)) continue;
                if (p.sub.isNotEmpty() && !subs.contains(p.sub))
                    subs.add(p.sub);
            }
        }
        if (cat == "All" || cat == "User" || favView || cat != "All")
        {
            for (const auto& u : userMetas)
            {
                if (cat != "All" && cat != "User" && !favView && u.cat != cat) continue;
                if (favView && !favorites.contains(u.name)) continue;
                if (u.sub.isNotEmpty() && !subs.contains(u.sub))
                    subs.add(u.sub);
            }
        }

        subs.sort(true);
        for (const auto& s : subs)
            subCategories.add(s);

        if (selSubCat >= subCategories.size()) selSubCat = 0;
        subCatList.updateContent();
        subCatList.repaint();
    }

    void rebuild()
    {
        entries.clear();
        const juce::String q = search.getText().toLowerCase().trim();
        if (selCat < 0 || selCat >= categories.size()) selCat = 0;
        const juce::String cat = categories[selCat];
        const bool favView = (cat == juce::String::fromUTF8(u8"Favorites ★"));
        const juce::String curSub = (selSubCat >= 0 && selSubCat < subCategories.size()) ? subCategories[selSubCat] : "All";

        auto match = [&](const juce::String& n, const juce::String& c, const juce::String& s)
        {
            if (q.isEmpty()) return true;
            return n.toLowerCase().contains(q)
                || c.toLowerCase().contains(q)
                || s.toLowerCase().contains(q);
        };

        auto accept = [&](const juce::String& n, const juce::String& c, const juce::String& s, bool isUser)
        {
            if (favView)
            {
                if (!favorites.contains(n)) return false;
            }
            else if (cat == "All")
            {
                // All passes
            }
            else if (cat == "User")
            {
                if (!isUser) return false;
            }
            else
            {
                if (c != cat) return false;
            }

            if (curSub != "All" && s != curSub) return false;
            if (!match(n, c, s)) return false;
            return true;
        };

        // Factory
        if (cat != "User")
        {
            const auto& all = GranularPresets::all();
            for (int i = 0; i < (int)all.size(); ++i)
            {
                const auto& p = all[(size_t)i];
                if (accept(p.name, p.category, p.sub, false))
                    entries.push_back({ true, i, {}, p.name, p.category, p.sub });
            }
        }

        // User
        auto userMetas = getUserMetas();
        for (const auto& u : userMetas)
        {
            if (accept(u.name, u.cat, u.sub, true))
                entries.push_back({ false, -1, u.file, u.name, u.cat, u.sub });
        }

        if (currentRow >= (int)entries.size()) currentRow = (int)entries.size() - 1;
        catList.updateContent();
        presetList.updateContent();
        repaint();
    }

    void loadRow(int row)
    {
        if (row < 0 || row >= (int)entries.size()) return;
        currentRow = row;
        presetList.selectRow(row);
        const auto& e = entries[(size_t)row];
        if (e.isFactory) { if (onLoadFactory) onLoadFactory(e.factoryIndex); }
        else             { if (onLoadUser) onLoadUser(e.file); }
    }

    void step(int dir)
    {
        if (entries.empty()) return;
        int row = (currentRow < 0) ? 0 : currentRow + dir;
        if (row < 0) row = (int)entries.size() - 1;
        if (row >= (int)entries.size()) row = 0;
        loadRow(row);
    }

    void randomizePreset()
    {
        if (entries.empty()) return;
        int row = juce::Random::getSystemRandom().nextInt((int)entries.size());
        loadRow(row);
    }

    void showRowMenu(int row)
    {
        if (row < 0 || row >= (int)entries.size()) return;
        const auto e = entries[(size_t)row];
        juce::PopupMenu m;
        const bool fav = favorites.contains(e.name);
        m.addItem(1, fav ? "Remove from Favorites" : juce::String::fromUTF8(u8"Mark Favorite ★"));
        if (!e.isFactory) m.addItem(2, "Delete Preset...");
        m.showMenuAsync(juce::PopupMenu::Options(),
            [this, e](int res)
            {
                if (res == 1) toggleFavorite(e.name);
                else if (res == 2)
                {
                    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
                        "Delete Preset", "Delete \"" + e.name + "\" ?", "Delete", "Cancel", this,
                        juce::ModalCallbackFunction::create([this, e](int ok)
                        {
                            if (ok == 1)
                            {
                                e.file.deleteFile();
                                if (favorites.contains(e.name)) { favorites.removeString(e.name); saveFavorites(); }
                                updateSubCategories();
                                rebuild();
                            }
                        }));
                }
            });
    }

    // --- メインカテゴリ ListBoxModel ---
    struct CatModel : juce::ListBoxModel
    {
        PresetBrowser& o; explicit CatModel(PresetBrowser& b) : o(b) {}
        int getNumRows() override { return o.categories.size(); }
        void paintListBoxItem(int r, juce::Graphics& g, int w, int h, bool sel) override
        {
            if (r < 0 || r >= o.categories.size()) return;
            g.setColour(sel ? GranularColors::lavender.withAlpha(0.25f) : juce::Colours::transparentBlack);
            g.fillRect(0, 0, w, h);
            g.setColour(r == o.selCat ? GranularColors::text : GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(12.0f, r == o.selCat ? juce::Font::bold : juce::Font::plain)));
            g.drawText(o.categories[r], 10, 0, w - 14, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int r, const juce::MouseEvent&) override
        {
            if (r < 0 || r >= o.categories.size()) return;
            o.selCat = r;
            o.selSubCat = 0;
            o.currentRow = -1;
            o.updateSubCategories();
            o.rebuild();
        }
    };

    // --- サブカテゴリ ListBoxModel ---
    struct SubCatModel : juce::ListBoxModel
    {
        PresetBrowser& o; explicit SubCatModel(PresetBrowser& b) : o(b) {}
        int getNumRows() override { return o.subCategories.size(); }
        void paintListBoxItem(int r, juce::Graphics& g, int w, int h, bool sel) override
        {
            if (r < 0 || r >= o.subCategories.size()) return;
            g.setColour(sel ? GranularColors::lavender.withAlpha(0.25f) : juce::Colours::transparentBlack);
            g.fillRect(0, 0, w, h);
            g.setColour(r == o.selSubCat ? GranularColors::text : GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(12.0f, r == o.selSubCat ? juce::Font::bold : juce::Font::plain)));
            g.drawText(o.subCategories[r], 10, 0, w - 14, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int r, const juce::MouseEvent&) override
        {
            if (r < 0 || r >= o.subCategories.size()) return;
            o.selSubCat = r;
            o.currentRow = -1;
            o.rebuild();
        }
    };

    // --- プリセット ListBoxModel ---
    struct PresetModel : juce::ListBoxModel
    {
        PresetBrowser& o; explicit PresetModel(PresetBrowser& b) : o(b) {}
        int getNumRows() override { return (int)o.entries.size(); }
        void paintListBoxItem(int r, juce::Graphics& g, int w, int h, bool sel) override
        {
            if (r < 0 || r >= (int)o.entries.size()) return;
            const auto& e = o.entries[(size_t)r];
            g.setColour(sel ? GranularColors::mint.withAlpha(0.18f) : juce::Colours::transparentBlack);
            g.fillRect(0, 0, w, h);

            // ★ / ☆ アイコン
            const bool fav = o.favorites.contains(e.name);
            g.setColour(fav ? juce::Colour(0xFFFFD700) : GranularColors::textDim.withAlpha(0.35f));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawText(fav ? juce::String::fromUTF8(u8"★") : juce::String::fromUTF8(u8"☆"), 6, 0, 18, h, juce::Justification::centred);

            // プリセット名
            g.setColour(GranularColors::text);
            g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
            g.drawText(e.name, 28, 0, w - 210, h, juce::Justification::centredLeft);

            // タグ (Category / Sub)
            g.setColour(GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            juce::String tag = e.cat;
            if (e.sub.isNotEmpty()) tag += "  /  " + e.sub;
            if (!e.isFactory) tag += " (User)";
            g.drawText(tag, w - 190, 0, 180, h, juce::Justification::centredRight);
        }
        void listBoxItemClicked(int r, const juce::MouseEvent& e) override
        {
            if (r < 0 || r >= (int)o.entries.size()) return;
            if (e.x <= 26) // ★ アイコンクリックで即座にお気に入りトグル
            {
                o.toggleFavorite(o.entries[(size_t)r].name);
                return;
            }
            if (e.mods.isPopupMenu())
                o.showRowMenu(r);
            else
                o.loadRow(r);
        }
    };

    CatModel catModel { *this };
    SubCatModel subCatModel { *this };
    PresetModel presetModel { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
