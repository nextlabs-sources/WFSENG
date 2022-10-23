#!/usr/bin/perl
#
# DESCRIPTION
# This script prepare an installer assembly directory for building an installer.
#
# IMPORTANT:
#	1. "Script Files" folder must be in the same directory as *.ism file. Otherwise, you will get
#		an error message like this:
#			ISDEV : error -7132: An error occurred streaming ISSetup.dll support file 
#			c:\nightly\current\D_SiriusR2\install\oe\desktop-enforcer-5.5.0.0-10001-dev-20110321063014\
#			desktop-enforcer-5.5.0.0-10001-dev-20110321063014\Script Files\Setup.inx
#		And you will not see these messages at the beginning:
#			Compiling...
#			Setup.rul
#			c:\nightly\current\D_SiriusR2\install\oe\desktop-enforcer-5.5.0.0-10001-dev-20110321063014\script files\Setup.rul(90) 
#				: warning W7503: 'ProcessEnd' : function defined but never called
#			c:\nightly\current\D_SiriusR2\install\oe\desktop-enforcer-5.5.0.0-10001-dev-20110321063014\script files\Setup.rul(90) 
#				: warning W7503: 'ProcessRunning' : function defined but never called
#			Linking...
#			Setup.inx - 0 error(s), 2 warning(s)
#			ISDEV : warning -4371: There were warnings compiling InstallScript


BEGIN { push @INC, "../scripts"}

use strict;
use warnings;

use Getopt::Long;
use File::Copy::Recursive qw(dircopy);
use InstallHelper;


print "NextLabs Installer Assembly Preparation Script\n";


#
# Global variables
#

my	$buildType = "";
my	$buildNum = "";
my	$ismTemplateFileName = "";
my	$versionStr = "";
my	$majorVer = 0;
my	$minorVer = 0;
my	$msiFilePrefix = "windowsfileserver-enforcer";


#
# Process parameters
#

# -----------------------------------------------------------------------------
# Print usage

sub printUsage
{
	print "usage: prepareAssembly.pl --buildType=<type> --buildNum=<#> --version=<string>\n";
	print "         --template=<file>\n";
	print "  buildNum        A build number. Can be any numerical or string value.\n";
	print "  buildType       Specify a build type (e.g., release, pcv, nightly or dev)\n";
	print "  template        Name of an InstallShield build script (.ism file).\n";
	print "  version         Version string of format major.minor (e.g., 5.5)\n";
	print "\nEnvironment Variables:\n";
	print "  NLBUILDROOT     Source tree root (e.g., c:/nightly/current/D_SiriusR2).\n";
}

# -----------------------------------------------------------------------------
# Parse command line arguments

sub parseCommandLine()
{
	#
	# Parse arguments
	#
	
	# GetOptions() key specification:
	#	option			Given as --option of not at all (value set to 0 or 1)
	#	option!			May be given as --option or --nooption (value set to 0 or 1)
	#	option=s		Mandatory string parameter: --option=somestring
	#	option:s		Optional string parameter: --option or --option=somestring	
	#	option=i		Mandatory integer parameter: --option=35
	#	option:i		Optional integer parameter: --option or --option=35	
	#	option=f		Mandatory floating point parameter: --option=3.14
	#	option:f		Optional floating point parameter: --option or --option=3.14	

	my	$help = 0;
		
	if (!GetOptions(
			'buildNum=s' => \$buildNum,				# --buildNum
			'buildType=s' => \$buildType,			# --buildType
			'help' => \$help,						# --help
			'template=s' => \$ismTemplateFileName,	# --template
			'version=s' => \$versionStr				# --version
		))
	{
		exit(1);
	}

	#
	# Help
	#
	
	if ($help == 1)
	{
		&printHelp();
		exit;
	}

	#
	# Check for errors
	#
	
	if ($buildType eq '')
	{
		print "Missing build type\n";
		exit(1);
	}

	if ($buildType ne "release" && $buildType ne "pcv" && $buildType ne "nightly" && $buildType ne "dev")
	{
		print "Invalid build type $buildType (expected release, pcv, nightly or dev)\n";
		exit(1);
	}
	
	if ($buildNum eq '')
	{
		print "Missing build number\n";
		exit(1);
	}

	if ($ismTemplateFileName eq '')
	{
		print "Missing ISM template file name\n";
		exit(1);
	}	
	
	if ($versionStr eq '')
	{
		print "Missing version string\n";
		exit(1);
	}
	
	if ($versionStr !~ /^(\d+)\.(\d+)$/)
	{
		print "Invalid verison string (expects format 5.5)\n";
		exit(1);
	}
	
	$majorVer = $1;
	$minorVer = $2;
		
	if ($majorVer < 1 || $majorVer > 100)
	{
		print "Invalid major verison # (expects 1-100)\n";
		exit(1);
	}
	
	if ($minorVer < 0 || $minorVer > 100)
	{
		print "Invalid minor verison # (expects 1-100)\n";
		exit(1);
	}
}

my	$argCount = scalar(@ARGV);

if ($argCount < 2 || $ARGV[0] eq "-h" || $ARGV[0] eq "--help")
{
	printUsage;
	exit 1;
}

&parseCommandLine();

# Print parameters
print "Parameters:\n";
print "  Build Type          = $buildType\n";
print "  Build #             = $buildNum\n";
print "  Version String      = $versionStr\n";
print "  Major               = $majorVer\n";
print "  Minor               = $minorVer\n";
print "  Template File Name  = $ismTemplateFileName\n";


#
# Environment
#

my	$buildRootDir = $ENV{NLBUILDROOT};
my	$buildRootPath = $buildRootDir;

$buildRootPath =~ s/:$/:\//;

if (! defined $buildRootDir || $buildRootDir eq "")
{
	die "### ERROR: Environment variable NLBUILDROOT is missing.\n";
}


if (! -d $buildRootPath)
{
	die "### ERROR: $buildRootPath (i.e., NLBUILDROOT) does not exists.\n";
}

# Print environment
print "Environment Variables:\n";
print "  NLBUILDROOT     = $buildRootDir\n";



#
# Construct names
#

print "INFO: Construct ISM and MSI names\n";

# Construct names
my	$ismFileBaseName = "";
my	$msiFileName32 = "";
my	$msiFileName64 = ""; 

if ($buildType eq "release")
{
	($ismFileBaseName, $msiFileName32, $msiFileName64) = InstallHelper::makeInstallerNames_Release($msiFilePrefix, $versionStr);
}
else
{
	($ismFileBaseName, $msiFileName32, $msiFileName64) = InstallHelper::makeInstallerNames_PreRelease($msiFilePrefix, $versionStr, $buildNum, $buildType);
}

# Print names
print "Assembly Names:\n";
print "  ISM file basename      = $ismFileBaseName\n";
print "  MSI file name (32-bit) = $msiFileName32\n";
print "  MSI file name (64-bit) = $msiFileName64\n";


my	$installDir = "$buildRootDir/install/scripts/wfse";
my  $assemblyDir = "$installDir/build/data";


#
# Copy build binaries
#

my	$srcBin64Dir = "$buildRootDir/bin/release_win_x64";
my	$releaseOnly = 0;

if ($buildType eq "release")
{
	$releaseOnly = 1;
}

#
# Prepare InstallShield files
#

# ***** TEMPORARY WORKAROUND BEGIN -- SHOULD BE REMOVED AFTER ISM SCRIPT IS FIXED *****
# Setup desired structure
my	$ismTemplateFile = "$installDir/$ismTemplateFileName";

#InstallHelper::copyRequired("$ismTemplateFile",							"$installDir/$ismTemplateFileName");

print "INFO: Copying InstallShield files\n";

my	$srcScriptDir = "$installDir/WindowsFileServerEnforcer";
my	$destScriptDir = "$assemblyDir/WindowsFileServerEnforcer";

InstallHelper::copyRequired("$installDir/$ismTemplateFileName",		"$assemblyDir/$ismTemplateFileName");

#dircopy($srcScriptDir, $destScriptDir) || die "### ERROR: Failed to copy resource from $srcScriptDir to $destScriptDir.\n";
if (system("cp -pfR \"$srcScriptDir\" \"$destScriptDir\""))
{
	die "### ERROR: Failed to copy resource from $srcScriptDir to $destScriptDir.\n";
}

#
# Modify installer build script
#

print "INFO: modify installer script\n";

my	$templateFile = "$installDir/$ismTemplateFileName";
my	$newIsmFile = "$assemblyDir/$ismTemplateFileName";

InstallHelper::constructIsmFile($templateFile, $newIsmFile, $msiFileName32, $msiFileName64, $versionStr, $buildNum);
