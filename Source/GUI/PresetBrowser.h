// ==========================================
// File: PresetBrowser.h
// プリセットブラウザ（Factory 80 + User + Favorites）。Mainタブ下の領域に展開。
//   - 左: カテゴリ（All / Favorites / 各カテゴリ / User）
//   - 右: プリセット一覧（検索フィルタ、クリックで即ロード、右クリックでメニュー）
//   - 上: 検索 / ▲▼ / 保存名 / RANDOM / SAVE / INIT / CLOSE
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
    std::function<void(int)>                 onLoadFactory;
    std::function<void(const juce::File&)>   onLoadUser;
    std::function<void(const juce::String&)> onSave;
    std::function<void()>                    onInit;
    std::function<void()>                    onRandom;
    std::function<void()>                    onClose;

    explicit PresetBrowser(juce::File presetDir) : userDir(presetDir)
    {
        setWantsKeyboardFocus(true);
        loadFavorites();

        categories.add("All");
        categories.add("Favorites");
        for (const auto& p : GranularPresets::all())
            if (!categories.contains(p.category)) categories.add(p.category);
        categories.add("User");

        catList.setModel(&catModel);      catList.setRowHeight(24);
        catList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        catList.setWantsKeyboardFocus(false);
        addAndMakeVisible(catList);

        presetList.setModel(&presetModel); presetList.setRowHeight(24);
        presetList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        presetList.setWantsKeyboardFocus(false); // ↑↓はブラウザ側で処理
        addAndMakeVisible(presetList);

        auto styleEdit = [](juce::TextEditor& e, const juce::String& hint)
        {
            e.setTextToShowWhenEmpty(hint, GranularColors::textDim);
            e.setColour(juce::TextEditor::backgroundColourId, GranularColors::knobTrack);
            e.setColour(juce::TextEditor::textColourId, GranularColors::text);
        };
        styleEdit(search, "Search...");   search.onTextChange = [this] { rebuild(); };
        styleEdit(saveName, "Preset name...");
        addAndMakeVisible(search);
        addAndMakeVisible(saveName);

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
        randBtn.onClick   = [this] { randomizePreset(); };
        saveBtn.onClick   = [this] { if (onSave && saveName.getText().trim().isNotEmpty()) { onSave(saveName.getText().trim()); rebuild(); } };
        initBtn.onClick   = [this] { if (onInit) onInit(); };
        closeBtn.onClick  = [this] { if (onClose) onClose(); };

        rebuild();
    }

    void refresh() { rebuild(); }

    void visibilityChanged() override { if (isVisible()) grabKeyboardFocus(); }

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
        g.setColour(GranularColors::lavender);
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText("PRESETS", 12, 6, 80, 18, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8);
        auto top = r.removeFromTop(30);
        top.removeFromLeft(80); // title
        search.setBounds(top.removeFromLeft(180).reduced(2));
        // 右側から順に配置
        closeBtn.setBounds(top.removeFromRight(64).reduced(2));
        initBtn.setBounds(top.removeFromRight(56).reduced(2));
        saveBtn.setBounds(top.removeFromRight(56).reduced(2));
        saveName.setBounds(top.removeFromRight(160).reduced(2));
        nextBtn.setBounds(top.removeFromRight(28).reduced(2));
        prevBtn.setBounds(top.removeFromRight(28).reduced(2));
        randBtn.setBounds(top.removeFromRight(64).reduced(2));

        r.removeFromTop(6);
        catList.setBounds(r.removeFromLeft(150));
        r.removeFromLeft(8);
        presetList.setBounds(r);
    }

private:
    struct Entry { bool isFactory; int factoryIndex; juce::File file; juce::String name, cat, sub; };

    juce::File userDir;
    juce::StringArray categories, favorites;
    int selCat = 0;
    int currentRow = -1;
    std::vector<Entry> entries;

    juce::ListBox catList, presetList;
    juce::TextEditor search, saveName;
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

    void rebuild()
    {
        entries.clear();
        const juce::String q = search.getText().toLowerCase();
        const juce::String cat = categories[selCat];
        auto match = [&](const juce::String& n) { return q.isEmpty() || n.toLowerCase().contains(q); };
        const bool favView = (cat == "Favorites");

        if (cat != "User")
        {
            const auto& all = GranularPresets::all();
            for (int i = 0; i < (int)all.size(); ++i)
            {
                const auto& p = all[(size_t)i];
                if (cat != "All" && !favView && p.category != cat) continue;
                if (favView && !favorites.contains(p.name)) continue;
                if (!match(p.name)) continue;
                entries.push_back({ true, i, {}, p.name, p.category, p.sub });
            }
        }
        if (cat == "All" || cat == "User" || favView)
        {
            if (userDir.isDirectory())
                for (auto& f : userDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
                {
                    const juce::String nm = f.getFileNameWithoutExtension();
                    if (favView && !favorites.contains(nm)) continue;
                    if (!match(nm)) continue;
                    entries.push_back({ false, -1, f, nm, "User", "" });
                }
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
        m.addItem(1, fav ? "Remove from Favorites" : "Add to Favorites");
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
                            if (ok == 1) { e.file.deleteFile(); if (favorites.contains(e.name)) { favorites.removeString(e.name); saveFavorites(); } rebuild(); }
                        }));
                }
            });
    }

    // --- カテゴリ ListBoxModel ---
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
        void listBoxItemClicked(int r, const juce::MouseEvent&) override { o.selCat = r; o.currentRow = -1; o.rebuild(); }
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
            const bool fav = o.favorites.contains(e.name);
            if (fav) { g.setColour(GranularColors::peach); g.setFont(juce::Font(juce::FontOptions(13.0f))); g.drawText("*", 8, 0, 14, h, juce::Justification::centredLeft); }
            g.setColour(GranularColors::text);
            g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
            g.drawText(e.name, 24, 0, w - 200, h, juce::Justification::centredLeft);
            g.setColour(GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            const juce::String tag = e.isFactory ? (e.cat + (e.sub.isNotEmpty() ? "  /  " + e.sub : juce::String()))
                                                  : juce::String("User");
            g.drawText(tag, w - 176, 0, 168, h, juce::Justification::centredRight);
        }
        void listBoxItemClicked(int r, const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) o.showRowMenu(r);
            else o.loadRow(r);
        }
    };

    CatModel catModel { *this };
    PresetModel presetModel { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
