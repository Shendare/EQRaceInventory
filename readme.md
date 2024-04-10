EverQuest Race Inventory - EQEmu <!-- body { font-family:Calibri,Tahoma,sans-serif; cursor:default; } code { border:1px solid black; background:#EEEEEE; padding:6px; } blockquote, li, p, h2, h3 { cursor:default; } -->

EverQuest Race Inventory
------------------------

**Current Version:** 1.1  
**Last Updated:** 05/25/2015

Files available for download:

*   [References - RoF2](EQRI_References_RoF2.zip)
*   [References - SoF](EQRI_References_SoF.zip)
*   [References - Titanium](EQRI_References_Titanium.zip)
*   [Program](EQRI.zip)
*   [Source Code](EQRI_Source.zip) (Free Visual Studio 2013 Community Edition)
*   [Old Source Code](EQRI_Source_VC2008.zip) (Titanium+SoF, Visual Studio Standard 2008)

Or you can view the clean install results directly:

*   [Race Data - RoF2](RoF2_EQRaces.htm)
*   [Zone Data - RoF2](RoF2_EQZones.htm)
*   [Race Data - SoF](SoF_EQRaces.htm)
*   [Zone Data - SoF](SoF_EQZones.htm)
*   [Race Data - Titanium](Titanium_EQRaces.htm)
*   [Zone Data - Titanium](Titanium_EQZones.htm)

### How to use the program

> Note: Only Titanium, SoF, and RoF2 (Rain of Fear, Steam Release #2) are currently supported!
> 
> Unzip EQRI.exe into your EQ directory, or create a new folder inside your EQ directory called "EQRI", and unzip the program into that. Technically, it can be unzipped to and run from anywhere, with the following caveat:
> 
> > If you decided to unzip it outside of your EQ directory, you'll need to create a EQRI.ini file in the folder EQRI.exe is in, telling it where to find your EQ installation, as follows:
> > 
> > As a relative path, such as from the EQRI folder inside your EverQuest directory...
> > 
> > `EQPath=..`
> > 
> > Or as a full path to your EQ installation, such as...
> > 
> > `EQPath=C:\Program Files\Sony\EverQuest`
> > 
> > Then the program will know where to look for the EQ files if it can't find them in its current directory.
> 
> Either way, it's just a matter of running EQRI.exe, either with a double-click or from the command line.
> 
> After less than a minute, you'll have a fresh set of references in the EQRI folder detailing the race/model availability as currently configured in your EQ installation (Version\_EQRaces.htm and Version\_EQZones.htm).
> 
> Press any key once the program is finished to exit.
> 
> You'll see the important info echoed to the console, while a very detailed log file is generated in the EQRI folder along with the references.
> 
> The references are built completely from the EQ source files. If you make a change to a ZoneName\_chr.txt file or Resources\\GlobalLoad.txt, the changes will be reflected in the results!
> 
> NOTE: The program must have write permission in the EQ directory to create the output files in the EQRI folder. In some Windows versions and configurations, this may require you to run the program as an Administrator. Alternatively, you can specify a different write-enabled output folder path in the EQRI.ini file as follows:
> 
> `OutPath=C:\OutputFolder`

### How it works

> #### Preparation
> 
> 1.  Reads textual information from eqstr\_us.txt and dbstr\_us.txt for reference later
> 2.  Reads eqgame.exe's hard-coded list of race/gender model codes by parsing the appropriate block of its machine code
> 3.  Reads eqgame.exe's hard-coded list of zones and their associated information fields (nick, name, expansion, minimum level, etc.) similarly
> 
> #### Model Availability Loading
> 
> 1.  Uses a hard-coded list of Luclin-enabled models (global5\_chr\[2\], frog\_mount\_chr, and globalXXX\_chr\[2\] for playable races)
> 2.  Parses Resources\\GlobalLoad.txt for remaining global model sources
> 3.  Using the Zone list, parses all ZoneNick\_chr\[2\].s3d files for zone-local models
> 4.  Using the Zone list, parses all ZoneNick\_chr.txt files for zone models imported from EQGs, S3Ds, and other zones
> 5.  Looks for any race/gender models that aren't being referenced in a .S3D or ZoneNick\_chr.txt file ("orphan models"), and looks for a ModelCode\_chr.s3d or ModelCode.eqg file for it
> 
> #### Model Information Loading
> 
> For every race/gender model it comes across:
> 
> 1.  Logs the source file it came from and the zone it's available in
> 2.  Parses an S3D file's inner .WLD file to log each model's Texture and Head (AKA HelmTexture) variations
>     *   Note: S3D/WLD parsing is monumentally more complicated than it sounds. The WLD format is cryptic, clunky, ambiguous, and difficult to load consistent, reliable information from. Most of the development time on this program was devoted to figuring out how to interpret the WLD data. Mad props to Windcatcher for his wlddoc.pdf and Pascal source code, which made this possible!
> 3.  Parses EQG files and interprets Textures, Heads, and other variations based on texture filenames used
>     *   Note: I didn't find file format information for the .MOD and .MDS files that make up EQG race models. Fortunately, the devs were decently consistent with the filename convention for textures used in models, and I was able to extrapolate everything needed for Texture and Head variations from those, with a little interpreting.
> 
> #### Reporting
> 
> 1.  Races: Builds a reference HTML file of all races the client can render, listing model codes, variations, sources, and the zones configured to load each model
> 2.  Zones: Builds a reference HTML file of all zones the client can handle, listing names, expansion requirements, bit flags, minimum level to enter, and what race models are available for rendering
> 3.  Zones: Also lists all global models and sources, along with their variations and what makes them global (Luclin models loaded, Resources\\GlobalLoad.txt, etc.)

### Conclusion

And that's it. Enjoy!

Of course, no guarantees of any kind are made as to the safety and usability of this program, though I personally use it and it hasn't messed anything up for me. It only reads the EQ files, never writes to them.

If you find something that doesn't seem to match up in-game, or if you seem to have run across a glitch, drop me a note on the EQEmu forums.

\- Jon, AKA Shendare