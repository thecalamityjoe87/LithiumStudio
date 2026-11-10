#include "Installation.h"

#include <juce_cryptography/juce_cryptography.h>

namespace lithium::installer
{

namespace
{
    const juce::String appId = "io.github.thecalamityjoe87.LithiumStudio";
    const juce::String appFileName = "Lithium Studio";

    juce::File home()   { return juce::File::getSpecialLocation (juce::File::userHomeDirectory); }

    // The menu entry. Exec is quoted because the path has a space in it, and
    // %f passes a session the user double-clicked. StartupWMClass is JUCE's
    // X11 window class (the app name), which lets the desktop match the open
    // window to this entry and its icon.
    juce::String desktopEntry (const juce::File& app, bool fileTypes)
    {
        return "[Desktop Entry]\n"
               "Type=Application\n"
               "Name=Lithium Studio\n"
               "GenericName=Music Studio\n"
               "Comment=Make music with an easy, friendly studio\n"
               "Exec=\"" + app.getFullPathName() + "\" %f\n"
               "Icon=" + appId + "\n"
               "Terminal=false\n"
               "Categories=AudioVideo;Audio;Music;Midi;\n"
               "Keywords=DAW;music;MIDI;synth;recording;sequencer;\n"
               + juce::String (fileTypes ? "MimeType=application/x-lithium-session;application/x-ikaros-session;\n" : "")
               + "StartupNotify=true\n"
                 "StartupWMClass=Lithium Studio\n"
                 "SingleMainWindow=true\n";
    }

    // Remove refuses paths that aren't ours: inside the home folder, and with
    // "lithium" somewhere in them. The manifest is a file the user could edit;
    // this keeps a damaged one from deleting anything else.
    bool safeToDelete (const juce::File& f)
    {
        return f.isAChildOf (home()) && f.getFullPathName().containsIgnoreCase ("lithium");
    }

    // Runs a desktop helper if it's there (it isn't on every system; nothing
    // breaks without it, the desktop just notices a little later).
    void runIfPresent (const juce::StringArray& command)
    {
        for (const auto* dir : { "/usr/bin", "/bin", "/usr/local/bin" })
        {
            if (juce::File (dir).getChildFile (command[0]).existsAsFile())
            {
                juce::ChildProcess process;

                if (process.start (command, 0))
                    process.waitForProcessToFinish (20000);

                return;
            }
        }
    }

    // Nudges the desktop to re-read menu entries, icons and file types.
    // Desktops watch these folders' modification times, so touching them is
    // enough for the menu and icons. (Not gtk-update-icon-cache: a cache file
    // left in the user's icon folder goes stale and hides icons other apps add
    // later.) File types need the MIME database rebuilt.
    void refreshDesktop()
    {
        const auto data = Installation::dataHome();

        for (const auto* folder : { "applications", "icons/hicolor" })
            if (const auto f = data.getChildFile (folder); f.isDirectory())
                f.setLastModificationTime (juce::Time::getCurrentTime());

        if (const auto mime = data.getChildFile ("mime"); mime.getChildFile ("packages").isDirectory())
            runIfPresent ({ "update-mime-database", mime.getFullPathName() });

        if (const auto apps = data.getChildFile ("applications"); apps.isDirectory())
            runIfPresent ({ "update-desktop-database", apps.getFullPathName() });
    }
}

const std::vector<Part>& components()
{
    static const std::vector<Part> list {
        { "app", "Lithium Studio", "The app, with an entry in your app menu.", "~/.local", true, true },
        // Required: new sessions and the Drummer use these sounds, and they're only 3 MB.
        { "sounds", "Lithium Basics sounds", "A small set of instruments and loops to start with.", "~/.local", true, true },
        { "filetypes", "Session files handler", "Double-click a session (.lisession) to open it in Lithium Studio.", "file manager", true, false },
        { "starter", "Starter sounds", "Piano, drums, bass, guitar and strings (about 380 MB, downloaded now).", "~/Documents", false, false },
    };
    return list;
}

Installation::Installation (juce::File p, juce::String v) : payload (std::move (p)), version (std::move (v)) {}

juce::File Installation::dataHome()
{
    const auto xdg = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
    return xdg.isNotEmpty() && juce::File::isAbsolutePath (xdg) ? juce::File (xdg) : home().getChildFile (".local/share");
}

juce::File Installation::programFolder()
{
    return home().getChildFile (".local/lib/lithium-studio");
}

juce::File Installation::soundsFolder()
{
    // SoundLibrary::getUserSoundsFolder: Documents/Lithium Studio/Sounds.
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Lithium Studio").getChildFile ("Sounds");
}

juce::File Installation::manifestFile()
{
    return programFolder().getChildFile ("install.json");
}

juce::File Installation::installedApp() const
{
    const auto app = programFolder().getChildFile (appFileName);
    return app.existsAsFile() ? app : juce::File();
}

juce::String Installation::checkPayload() const
{
    for (const auto path : { "app/" + appFileName, juce::String ("app/Sounds"), juce::String ("icons"),
                             appId + ".xml", juce::String ("LICENSE") })
        if (! payload.getChildFile (path).exists())
            return "The installer's files are incomplete (no " + path + " in " + payload.getFullPathName()
                   + "). Unpack the whole download again and run the installer from there.";

    return {};
}

Existing Installation::existing() const
{
    Existing result;
    const auto json = juce::JSON::parse (manifestFile());

    if (auto* object = json.getDynamicObject())
    {
        result.installed = true;
        result.version = object->getProperty ("version").toString();

        if (auto* list = object->getProperty ("components").getArray())
            for (const auto& c : *list)
                result.components.add (c.toString());
    }

    return result;
}

juce::String Installation::remove (const Progress& progress)
{
    const auto json = juce::JSON::parse (manifestFile());
    juce::StringArray paths;

    if (auto* object = json.getDynamicObject())
        if (auto* list = object->getProperty ("paths").getArray())
            for (const auto& p : *list)
                paths.add (p.toString());

    juce::StringArray failed;

    for (int i = 0; i < paths.size(); ++i)
    {
        const juce::File f (paths[i]);
        progress ("Removing " + f.getFileName(), (float) i / (float) std::max (1, paths.size()));

        if (! safeToDelete (f))
            continue;

        // A symlink is deleted as a link, never followed.
        const auto ok = f.isSymbolicLink() || ! f.isDirectory() ? f.deleteFile() : f.deleteRecursively();

        if (! ok && f.exists())
            failed.add (f.getFullPathName());
    }

    manifestFile().deleteFile();

    // The program folder, if nothing else is left in it.
    if (programFolder().isDirectory() && programFolder().getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 0)
        programFolder().deleteFile();

    refreshDesktop();
    progress ("Removed", 1.0f);
    return failed.isEmpty() ? juce::String() : "Couldn't remove:\n" + failed.joinIntoString ("\n");
}

juce::String Installation::install (const juce::StringArray& ids, const Progress& reportProgress)
{
    warning = {};

    if (const auto error = checkPayload(); error.isNotEmpty())
        return error;

    // With the starter sounds, installing the app is the first fifth of the
    // bar and their download the rest.
    const bool starter = ids.contains ("starter");
    const Progress progress = [&] (const juce::String& s, float p) { reportProgress (s, starter ? p * 0.2f : p); };

    // A fresh start: an update shouldn't leave behind files an older version
    // had and this one doesn't.
    if (manifestFile().existsAsFile())
        if (const auto error = remove ([&] (const juce::String& s, float p) { progress (s, p * 0.1f); }); error.isNotEmpty())
            return error;

    // Each step: what to copy from the payload, and where to.
    struct Step { juce::String status; juce::File from, to; bool executable = false; };
    std::vector<Step> steps;
    const auto data = dataHome();
    const auto program = programFolder();
    const auto bin = home().getChildFile (".local/bin");
    const bool fileTypes = ids.contains ("filetypes");

    steps.push_back ({ "Copying Lithium Studio", payload.getChildFile ("app/" + appFileName), program.getChildFile (appFileName), true });
    steps.push_back ({ "Copying the Lithium Basics sounds", payload.getChildFile ("app/Sounds"), program.getChildFile ("Sounds") });

    for (const auto& icon : payload.getChildFile ("icons").findChildFiles (juce::File::findFiles, false, "icon-*.png"))
    {
        const auto size = icon.getFileNameWithoutExtension().fromFirstOccurrenceOf ("-", false, false);
        steps.push_back ({ "Adding the icons", icon, data.getChildFile ("icons/hicolor/" + size + "x" + size + "/apps/" + appId + ".png") });
    }

    if (const auto svg = payload.getChildFile ("icons/icon.svg"); svg.existsAsFile())
        steps.push_back ({ "Adding the icons", svg, data.getChildFile ("icons/hicolor/scalable/apps/" + appId + ".svg") });

    if (fileTypes)
        steps.push_back ({ "Adding the session file types", payload.getChildFile (appId + ".xml"),
                           data.getChildFile ("mime/packages/" + appId + ".xml") });

    for (const auto* doc : { "LICENSE", "CREDITS" })
        if (payload.getChildFile (doc).existsAsFile())
            steps.push_back ({ "Copying the license", payload.getChildFile (doc), program.getChildFile (doc) });

    juce::StringArray installed;

    for (size_t i = 0; i < steps.size(); ++i)
    {
        const auto& step = steps[i];
        progress (step.status + "...", 0.1f + 0.8f * (float) i / (float) steps.size());

        if (! step.from.exists())
            return "Missing from the installer: " + step.from.getFullPathName();

        step.to.getParentDirectory().createDirectory();

        // Delete first, then write a new file: a running copy of the app keeps
        // its (now unlinked) program instead of having it overwritten under it.
        step.to.isDirectory() ? step.to.deleteRecursively() : step.to.deleteFile();
        const auto ok = step.from.isDirectory() ? step.from.copyDirectoryTo (step.to) : step.from.copyFileTo (step.to);

        if (! ok)
            return "Couldn't write " + step.to.getFullPathName() + ". Is the disk full, or the folder read-only?";

        if (step.executable)
            step.to.setExecutePermission (true);

        installed.add (step.to.getFullPathName());
    }

    progress ("Adding Lithium Studio to the app menu...", 0.92f);
    const auto app = program.getChildFile (appFileName);
    const auto entry = data.getChildFile ("applications/" + appId + ".desktop");
    entry.getParentDirectory().createDirectory();

    if (! entry.replaceWithText (desktopEntry (app, fileTypes)))
        return "Couldn't write " + entry.getFullPathName();

    installed.add (entry.getFullPathName());

    // A command for the terminal too. ~/.local/bin is on the PATH on most
    // distributions; if it isn't, the menu entry still works.
    const auto link = bin.getChildFile ("lithium-studio");
    bin.createDirectory();

    if (app.createSymbolicLink (link, true))
        installed.add (link.getFullPathName());

    // The manifest last, so a failed install doesn't claim to be one.
    auto manifest = std::make_unique<juce::DynamicObject>();
    manifest->setProperty ("version", version);
    manifest->setProperty ("components", juce::var (ids));
    manifest->setProperty ("paths", juce::var (installed));
    program.createDirectory();

    if (! manifestFile().replaceWithText (juce::JSON::toString (juce::var (manifest.release()))))
        return "Couldn't write " + manifestFile().getFullPathName();

    progress ("Updating the app menu and file types...", 0.96f);
    refreshDesktop();

    // The app is installed now; the sounds are a bonus. If they can't be
    // downloaded, say so, but the install still counts.
    if (starter)
    {
        const auto error = installStarterSounds ([&] (const juce::String& s, float p) { reportProgress (s, 0.2f + 0.8f * p); });

        if (error.isNotEmpty())
            warning = "Lithium Studio is installed, but the starter sounds couldn't be downloaded (" + error
                      + "). You can download them in the app: Sounds > Manage Sounds.";
    }

    reportProgress ("Installed", 1.0f);
    return {};
}

//==============================================================================
// The starter sounds: as PackStore does it (src/engine/PackStore.cpp), for
// packs in the app's own format: read the Lithium Sounds index, then for each
// pack in its "starter" set download the zip, check its SHA-256, unpack it
// into a hidden folder and swap it in.
namespace
{
    juce::String readPackId (const juce::File& packFolder)
    {
        return juce::JSON::parse (packFolder.getChildFile ("pack.json"))["id"].toString();
    }

    juce::String readPackVersion (const juce::File& packFolder)
    {
        return juce::JSON::parse (packFolder.getChildFile ("pack.json"))["version"].toString();
    }

    juce::File findInstalledPack (const juce::File& sounds, const juce::String& id)
    {
        for (auto& f : sounds.findChildFiles (juce::File::findDirectories | juce::File::ignoreHiddenFiles, false))
            if (readPackId (f) == id)
                return f;

        return {};
    }

    juce::String download (const juce::URL& url, const juce::File& to, juce::int64 expectedSize,
                           const std::function<void (float)>& progress)
    {
        int status = 0;
        auto in = url.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                             .withConnectionTimeoutMs (20000)
                                             .withStatusCode (&status));

        if (in == nullptr || (status != 0 && status != 200))
            return "no internet connection, or the sounds aren't available right now";

        to.deleteFile();
        juce::FileOutputStream out (to);

        if (! out.openedOk())
            return "couldn't write to " + to.getParentDirectory().getFullPathName();

        const auto total = in->getTotalLength() > 0 ? in->getTotalLength() : expectedSize;
        juce::HeapBlock<char> buffer (65536);
        juce::int64 done = 0;

        while (! in->isExhausted())
        {
            const auto got = in->read (buffer, 65536);

            if (got < 0)
                return "the download stopped";

            if (got == 0)
                break;

            if (! out.write (buffer, (size_t) got))
                return "the disk is full";

            done += got;
            progress (total > 0 ? juce::jlimit (0.0f, 1.0f, (float) done / (float) total) : 0.0f);
        }

        out.flush();
        return out.getStatus().wasOk() ? juce::String() : "the disk is full";
    }

    // Unpacks a pack zip into `staging` and returns its folder there. Every
    // entry must be inside one folder: no absolute paths, no "..", no links.
    juce::String unpack (const juce::File& zipFile, const juce::File& staging, const juce::String& packId, juce::File& packFolder)
    {
        juce::ZipFile zip (zipFile);
        juce::String top;

        for (int i = 0; i < zip.getNumEntries(); ++i)
        {
            auto* entry = zip.getEntry (i);
            const auto name = entry->filename.replaceCharacter ('\\', '/');
            const auto first = name.upToFirstOccurrenceOf ("/", false, false);

            if (entry->isSymbolicLink || name.startsWith ("/") || name.contains (":")
                 || name.contains ("../") || name.endsWith ("..") || ! name.contains ("/")
                 || first.isEmpty() || first.startsWith (".") || (top.isNotEmpty() && first != top))
                return "a download wasn't laid out the way it should be";

            top = first;
        }

        staging.deleteRecursively();

        if (top.isEmpty() || ! staging.createDirectory())
            return "a download was empty";

        if (auto result = zip.uncompressTo (staging); result.failed())
            return "couldn't unpack a download: " + result.getErrorMessage();

        packFolder = staging.getChildFile (top);
        return readPackId (packFolder) == packId ? juce::String() : "a download wasn't the pack it should be";
    }

    // Swaps the new pack in, replacing an installed older version.
    juce::String moveIntoPlace (const juce::File& sounds, const juce::File& packFolder, const juce::String& packId)
    {
        const auto old = findInstalledPack (sounds, packId);
        const auto aside = sounds.getChildFile (".old-" + packId);
        aside.deleteRecursively();

        if (old.exists() && ! old.moveFileTo (aside))
            return "couldn't replace an installed pack (is a file in it open?)";

        auto destination = sounds.getChildFile (packFolder.getFileName());

        if (destination.exists())   // a different pack with the same folder name
            destination = sounds.getNonexistentChildFile (packFolder.getFileName(), {}, true);

        if (! packFolder.moveFileTo (destination))
        {
            if (old != juce::File())
                aside.moveFileTo (old);

            return "couldn't install a pack in " + sounds.getFullPathName();
        }

        aside.deleteRecursively();
        return {};
    }
}

juce::String Installation::installStarterSounds (const Progress& progress)
{
    progress ("Finding the starter sounds...", 0.0f);

    const juce::URL indexUrl (LITHIUM_SOUNDS_INDEX_URL);
    int status = 0;
    auto in = indexUrl.createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                              .withConnectionTimeoutMs (20000)
                                              .withStatusCode (&status));

    if (in == nullptr || (status != 0 && status != 200))
        return "no internet connection, or the sounds aren't available right now";

    const auto index = juce::JSON::parse (in->readEntireStreamAsString());

    // The packs in the "starter" set, as the index lists them.
    juce::Array<juce::var> packs;

    if (auto* collections = index["collections"].getArray())
        for (auto& c : *collections)
            if (c["id"].toString() == "starter")
                if (auto* ids = c["packs"].getArray())
                    for (auto& id : *ids)
                        if (auto* list = index["packs"].getArray())
                            for (auto& p : *list)
                                if (p["id"].toString() == id.toString())
                                    packs.add (p);

    if (packs.isEmpty())
        return "the sound list has no starter sounds";

    const auto sounds = soundsFolder();

    if (! sounds.createDirectory())
        return "couldn't create " + sounds.getFullPathName();

    juce::int64 total = 0, done = 0;

    for (auto& p : packs)
        total += (juce::int64) p["downloadSize"];

    for (int i = 0; i < packs.size(); ++i)
    {
        const auto& p = packs.getReference (i);
        const auto id = p["id"].toString();
        const auto name = p["name"].toString();
        const auto size = (juce::int64) p["downloadSize"];
        const auto sha256 = p["sha256"].toString().toLowerCase();
        const auto counter = " (" + juce::String (i + 1) + " of " + juce::String (packs.size()) + ")";

        // Already there, at this version: nothing to do.
        if (const auto installed = findInstalledPack (sounds, id); installed.exists() && readPackVersion (installed) == p["version"].toString())
        {
            done += size;
            continue;
        }

        // A relative address is next to the index.
        const auto address = p["url"].toString();
        const auto url = address.contains (":") ? juce::URL (address) : indexUrl.getParentURL().getChildURL (address);

        // Hidden names, so the library never mistakes these for packs.
        const auto zipFile = sounds.getChildFile (".download-" + id + ".zip");
        const auto staging = sounds.getChildFile (".installing-" + id);

        auto error = download (url, zipFile, size, [&] (float f)
        {
            progress ("Downloading " + name + counter + "...", total > 0 ? ((float) done + f * (float) size) / (float) total : 0.0f);
        });

        progress ("Installing " + name + counter + "...", total > 0 ? (float) (done + size) / (float) total : 0.0f);

        if (error.isEmpty() && juce::SHA256 (zipFile).toHexString() != sha256)
            error = "a download was damaged";

        juce::File packFolder;

        if (error.isEmpty())
            error = unpack (zipFile, staging, id, packFolder);

        if (error.isEmpty())
            error = moveIntoPlace (sounds, packFolder, id);

        zipFile.deleteFile();
        staging.deleteRecursively();

        if (error.isNotEmpty())
            return error;

        done += size;
    }

    return {};
}

}
