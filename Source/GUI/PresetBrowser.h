// ==========================================
// File: PresetBrowser.h
// プリセットブラウザ（Factory 80 + User）。Mainタブ下の領域に展開。
//   - 左: カテゴリ（All / 各カテゴリ / User）
//   - 右: プリセット一覧（検索フィルタ付き、クリックで即ロード）
//   - 上: 検索 / 保存名 / SAVE / INIT / CLOSE
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <vector>

#include "ColorPalette.h"
#include "../Presets/PresetData.h"

class PresetBrowser : public juce::Component
{
public:
    std::function<void(int)>                onLoadFactory;   // factory index
    std::function<void(const juce::File&)>  onLoadUser;      // user file
    std::function<void(const juce::String&)> onSave;         // save name
    std::function<void()>                   onInit;
    std::function<void()>                   onClose;

    explicit PresetBrowser(juce::File presetDir) : userDir(presetDir)
    {
        // カテゴリ収集（Factoryの登場順を保持）
        categories.add("All");
        for (const auto& p : GranularPresets::all())
            if (!categories.contains(p.category)) categories.add(p.category);
        categories.add("User");

        catList.setModel(&catModel);
        catList.setRowHeight(24);
        catList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        addAndMakeVisible(catList);

        presetList.setModel(&presetModel);
        presetList.setRowHeight(24);
        presetList.setColour(juce::ListBox::backgroundColourId, GranularColors::panel);
        addAndMakeVisible(presetList);

        search.setTextToShowWhenEmpty("Search...", GranularColors::textDim);
        search.setColour(juce::TextEditor::backgroundColourId, GranularColors::knobTrack);
        search.setColour(juce::TextEditor::textColourId, GranularColors::text);
        search.onTextChange = [this] { rebuild(); };
        addAndMakeVisible(search);

        saveName.setTextToShowWhenEmpty("Preset name...", GranularColors::textDim);
        saveName.setColour(juce::TextEditor::backgroundColourId, GranularColors::knobTrack);
        saveName.setColour(juce::TextEditor::textColourId, GranularColors::text);
        addAndMakeVisible(saveName);

        auto styleBtn = [this](juce::TextButton& b, juce::Colour c)
        {
            b.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
            b.setColour(juce::TextButton::textColourOffId, c);
            addAndMakeVisible(b);
        };
        styleBtn(saveBtn,  GranularColors::mint);
        styleBtn(initBtn,  GranularColors::text);
        styleBtn(closeBtn, GranularColors::pink);

        saveBtn.onClick  = [this] { if (onSave && saveName.getText().trim().isNotEmpty()) { onSave(saveName.getText().trim()); rebuild(); } };
        initBtn.onClick  = [this] { if (onInit) onInit(); };
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        rebuild();
    }

    void refresh() { rebuild(); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GranularColors::bg);
        g.setColour(GranularColors::panelLine);
        g.drawRect(getLocalBounds(), 1);

        g.setColour(GranularColors::lavender);
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText("PRESETS", 12, 6, 200, 18, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8);
        auto top = r.removeFromTop(30);
        top.removeFromLeft(80); // タイトル分
        search.setBounds(top.removeFromLeft(200).reduced(2));
        closeBtn.setBounds(top.removeFromRight(70).reduced(2));
        initBtn.setBounds(top.removeFromRight(70).reduced(2));
        saveBtn.setBounds(top.removeFromRight(70).reduced(2));
        saveName.setBounds(top.removeFromRight(180).reduced(2));

        r.removeFromTop(6);
        catList.setBounds(r.removeFromLeft(160));
        r.removeFromLeft(8);
        presetList.setBounds(r);
    }

private:
    struct Entry { bool isFactory; int factoryIndex; juce::File file; juce::String name, cat, sub; };

    juce::File userDir;
    juce::StringArray categories;
    int selCat = 0;
    std::vector<Entry> entries;

    juce::ListBox catList, presetList;
    juce::TextEditor search, saveName;
    juce::TextButton saveBtn { "SAVE" }, initBtn { "INIT" }, closeBtn { "CLOSE" };

    void rebuild()
    {
        entries.clear();
        const juce::String q = search.getText().toLowerCase();
        const juce::String cat = categories[selCat];
        auto match = [&](const juce::String& n) { return q.isEmpty() || n.toLowerCase().contains(q); };

        if (cat != "User")
        {
            const auto& all = GranularPresets::all();
            for (int i = 0; i < (int)all.size(); ++i)
            {
                const auto& p = all[(size_t)i];
                if (cat != "All" && p.category != cat) continue;
                if (!match(p.name)) continue;
                entries.push_back({ true, i, {}, p.name, p.category, p.sub });
            }
        }
        if (cat == "All" || cat == "User")
        {
            if (userDir.isDirectory())
                for (auto& f : userDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
                {
                    const juce::String nm = f.getFileNameWithoutExtension();
                    if (!match(nm)) continue;
                    entries.push_back({ false, -1, f, nm, "User", "" });
                }
        }
        catList.updateContent();
        presetList.updateContent();
        repaint();
    }

    void loadRow(int row)
    {
        if (row < 0 || row >= (int)entries.size()) return;
        const auto& e = entries[(size_t)row];
        if (e.isFactory) { if (onLoadFactory) onLoadFactory(e.factoryIndex); }
        else             { if (onLoadUser) onLoadUser(e.file); }
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
        void listBoxItemClicked(int r, const juce::MouseEvent&) override { o.selCat = r; o.rebuild(); }
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
            g.setColour(GranularColors::text);
            g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
            g.drawText(e.name, 10, 0, w - 180, h, juce::Justification::centredLeft);
            g.setColour(GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            const juce::String tag = e.isFactory ? (e.cat + (e.sub.isNotEmpty() ? "  /  " + e.sub : juce::String()))
                                                  : juce::String("User");
            g.drawText(tag, w - 176, 0, 168, h, juce::Justification::centredRight);
        }
        void listBoxItemClicked(int r, const juce::MouseEvent&) override { o.loadRow(r); }
    };

    CatModel catModel { *this };
    PresetModel presetModel { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
