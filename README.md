# gitview
Total Commander plugin for browsing git repository branches

Plugin won't work in TC 5.5 and lower, since reading plugin settings is not available in earlier versions.

### Settings file
Name: gitview.json, should be in same directory as wincmd.ini or in plugins/ subdirectory.

Format:

	{
		"git client": {
			"git": "full/path/to/git.exe",
			"timeoutMs": "700", //timeout for git.exe results
			"unicodeIO": "yes", //if your branch names use non-ASCII characters and are not displayed correctly, try playing with this parameter
			"showCurrentBranch": "false" //show or don't show current branch (if shown, it is marked with '*')
		},

		"repos": [
			{
				"name": "name to appear in TC",
				"workingDir": "local working dir"
			},
			...
		],

		"debug": {
			"logLocation": "optional log location" //if settings file cannot be read for some reason, this is logged in gitview.log file in user home directory
		}
	}
