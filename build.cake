///////////////////////////////////////////////////////////////////////////////
// USINGS
///////////////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Linq;

///////////////////////////////////////////////////////////////////////////////
// ARGUMENTS
///////////////////////////////////////////////////////////////////////////////

var target = Argument("target", "Default");

///////////////////////////////////////////////////////////////////////////////
// SETTINGS
///////////////////////////////////////////////////////////////////////////////

var platforms = new [] { "Win32", "x64" }.ToList();
var configurations = new [] { "Release" }.ToList();
var solution = "./rewavi.sln";
var artifactsDir = (DirectoryPath)Directory("./artifacts");
var binDir = (DirectoryPath)Directory("./src/bin");
var objDir = (DirectoryPath)Directory("./src/obj");

///////////////////////////////////////////////////////////////////////////////
// RELEASE
///////////////////////////////////////////////////////////////////////////////

var isAppVeyorBuild = AppVeyor.IsRunningOnAppVeyor;
var isLocalBuild = BuildSystem.IsLocalBuild;
var isPullRequest = BuildSystem.AppVeyor.Environment.PullRequest.IsPullRequest;
var isMainRepo = StringComparer.OrdinalIgnoreCase.Equals("chikuzen/rewavi", BuildSystem.AppVeyor.Environment.Repository.Name);
var isMasterBranch = StringComparer.OrdinalIgnoreCase.Equals("master", BuildSystem.AppVeyor.Environment.Repository.Branch);
var isTagged = BuildSystem.AppVeyor.Environment.Repository.Tag.IsTag 
               && !string.IsNullOrWhiteSpace(BuildSystem.AppVeyor.Environment.Repository.Tag.Name);
var isRelease =  !isLocalBuild && !isPullRequest && isMainRepo && isMasterBranch && isTagged;

///////////////////////////////////////////////////////////////////////////////
// VERSION
///////////////////////////////////////////////////////////////////////////////

var version = "0.02";

Information("Version: {0}", version);

///////////////////////////////////////////////////////////////////////////////
// ACTIONS
///////////////////////////////////////////////////////////////////////////////

var buildSolutionAction = new Action<string,string,string> ((solution, configuration, platform) => 
{
    Information("Building: {0}, {1} / {2}", solution, configuration, platform);
    MSBuild(solution, settings => {
        settings.SetConfiguration(configuration);
        settings.WithProperty("Platform", "\"" + platform + "\"");
        settings.SetVerbosity(Verbosity.Minimal); });
});

var packageBinariesAction = new Action<string,string> ((configuration, platform) => 
{
    var output = "rewavi-" + version + "-" + platform + (configuration.Contains("Release") ? "" : ("-" + configuration));
    var outputDir = artifactsDir.Combine(output);
    var outputZip = artifactsDir.CombineWithFilePath(output + ".zip");
    var rewaviExeFile = File("./src/bin/rewavi/" + configuration + "/" + platform + "/" + "rewavi.exe");
    var resilenceExeFile = File("./src/bin/resilence/" + configuration + "/" + platform + "/" + "resilence.exe");
    CleanDirectory(outputDir);
    CopyFileToDirectory(File("README.md"), outputDir);
    CopyFileToDirectory(File("LICENSE.TXT"), outputDir);
    CopyFileToDirectory(File("./docs/usage.txt"), outputDir);
    CopyFileToDirectory(File("./docs/usage2.txt"), outputDir);
    CopyFileToDirectory(rewaviExeFile, outputDir);
    CopyFileToDirectory(resilenceExeFile, outputDir);
    Zip(outputDir, outputZip);
});

///////////////////////////////////////////////////////////////////////////////
// TASKS
///////////////////////////////////////////////////////////////////////////////

Task("Clean")
    .Does(() =>
{
    CleanDirectory(artifactsDir);
    CleanDirectory(binDir);
    CleanDirectory(objDir);
});

Task("Build")
    .IsDependentOn("Clean")
    .Does(() =>
{
    configurations.ForEach(configuration => platforms.ForEach(platform => buildSolutionAction(solution, configuration, platform)));
});

Task("Package-Binaries")
    .IsDependentOn("Build")
    .Does(() =>
{
    configurations.ForEach(configuration => platforms.ForEach(platform => packageBinariesAction(configuration, platform)));
});

///////////////////////////////////////////////////////////////////////////////
// TARGETS
///////////////////////////////////////////////////////////////////////////////

Task("Package")
  .IsDependentOn("Package-Binaries");

Task("Default")
  .IsDependentOn("Build");

///////////////////////////////////////////////////////////////////////////////
// EXECUTE
///////////////////////////////////////////////////////////////////////////////

RunTarget(target);
