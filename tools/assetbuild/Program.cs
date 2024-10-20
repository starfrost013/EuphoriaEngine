﻿// Assetbuild Utility
// Copyright © 2023-2024 starfrost
// 
// Code sucks lol
// Version 2.1.0 used for Euhp

#region Constants & Variables

const string ASSETBUILD_VERSION = "2.2.0"; // Version
const string DEFAULT_GAME_NAME = "examplegame"; // Default engine game name folder to use
string gameName = DEFAULT_GAME_NAME; // Name of the game to compile.
string gameDir = $@"..\..\..\..\..\game\assets\{gameName}"; // Complete relative path to game dir
string outputDirectory = string.Empty; //set later
string releaseConfiguration = string.Empty;

bool quietMode = false; // If true, everything except errors are hushed

#endregion

// Localise here!
#region Strings
const string STRING_SIGNON = $"Asset Build Tool {ASSETBUILD_VERSION}";
string STRING_DESCRIPTION = $"Builds assets for {gameName}";
const string STRING_USAGE = "Assetbuild <game> <release cfg> [-q]\n\n" +
    "<game>: Path to directory contaning game files\n" +
    "[directory]: Optional - base directory (default is ../../../../../assets/<game name>) ('_raw' after it for raw bsps, TEMP) \n" +
    "[release cfg]: Configuration to release the game for (Debug, Playtest or Release)\n" +
    "[-q]: Optional - quiets everything except errors";
string STRING_DELETING_OLD_FILES = $"Deleting old game files for {gameName}...";
string STRING_BUILDING_FILES = $"Building game files for {gameName}...";
const string STRING_BUILDING_DONE = "Done!";
string STRING_ERROR_NO_GAMEDIR = $"The base directory {gameDir} does not exist!"; // includes variable in variables section
const string STRING_ERROR_GENERIC = "An exception occurred: ";
#endregion

try
{
    // stupid hack for running under MSBuild
    Directory.SetCurrentDirectory(AppDomain.CurrentDomain.BaseDirectory);

    Print(STRING_SIGNON);
    Print(STRING_DESCRIPTION);

    #region Command-line parsing

    // set default name and release configuration if they haven't been overridden
    if (args.Length < 1)
        gameName = DEFAULT_GAME_NAME;

    if (args.Length < 2)
        releaseConfiguration = "Debug";

    if (args.Length >= 2)
    {
        if (!Directory.Exists(gameDir))
        {
            PrintLoud(STRING_ERROR_NO_GAMEDIR);
            PrintHelpAndExit(2);
        }

        gameName = args[0];
        releaseConfiguration = args[1];
    }

    // see if any of the user-provided arguments include quiet mode
    // skip the last argument
    for (int argNum = 0; argNum < args.Length - 1; argNum++)
    {
        string currentArg = args[argNum];

        if (string.Equals(currentArg, "-q", StringComparison.InvariantCultureIgnoreCase))
            quietMode = true;
    }

    // set the final directory
    outputDirectory = $@"..\..\..\..\..\build\{releaseConfiguration}\{gameName}"; // final directory

    // could be first build of a new game so just create final dir if it exists
    if (!Directory.Exists(outputDirectory)) Directory.CreateDirectory(outputDirectory);
    #endregion

    #region Main code

    // Delete old assets
    Print(STRING_DELETING_OLD_FILES);

    string[] finalFiles = Directory.GetFiles(outputDirectory, "*.*", SearchOption.AllDirectories);

    if (finalFiles.Length > 0)
    {
        foreach (string finalFile in finalFiles)
        {
            File.Delete(finalFile);
        }
    }

    Print(STRING_BUILDING_FILES);
    // get all files in the game dir
    string[] gameFiles = Directory.GetFiles(gameDir, "*.*", SearchOption.AllDirectories);

    // copy them all to the output directory

    // exclude raw assets
    string[] excludedPatterns = [".pdn", ".map", ".prt", ".log", ".pts", ".texinfo.json", ".ssv", @"save\", "save/", "save0",
        "scrnshot", "screenshot", "screenshots", ".skp", ".skb", ".mtl", ".obj", ".fbx"];

    foreach (string gameFile in gameFiles)
    {
        bool excluded = false;

        foreach (string excludedPattern in excludedPatterns)
        {
            if (gameFile.Contains(excludedPattern)) excluded = true;
        }

        // create the output dir for this file and then copy
        if (!excluded)
        {
            string relativePath = Path.GetRelativePath(gameDir, gameFile);

            string? finalDirectory = $@"{outputDirectory}\\{Path.GetDirectoryName(relativePath)}";

            if (!Directory.Exists(finalDirectory)
                && !string.IsNullOrWhiteSpace(finalDirectory)) Directory.CreateDirectory(finalDirectory);

            File.Copy(gameFile, $@"{outputDirectory}\{relativePath}", true);
        }

    }


    Print(STRING_BUILDING_DONE);
    Console.ResetColor();
    #endregion

}
catch (Exception ex)
{
    PrintErrorAndExit($"{STRING_ERROR_GENERIC} \n\n{ex}", 8);
}

#region Utility functions
void PrintHelpAndExit(int exitCode)
{
    Console.WriteLine(STRING_USAGE);
    Environment.Exit(exitCode);
}

/// Always print text, regardless of if quiet mode is on.
void PrintLoud(string text, ConsoleColor foreground = ConsoleColor.Gray)
{
    Console.ForegroundColor = foreground;
    Console.WriteLine(text);
    Console.ResetColor();
}

void Print(string text, ConsoleColor foreground = ConsoleColor.Gray)
{
    if (!quietMode) PrintLoud(text, foreground);
}

void PrintErrorAndExit(string errorString, int errorId)
{
    Console.ForegroundColor = ConsoleColor.Red;
    Console.WriteLine($"Error: {errorString}");
    Console.ResetColor();
    Environment.Exit(errorId);
}


#endregion