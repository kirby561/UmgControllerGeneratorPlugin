# UmgControllerGeneratorPlugin
This is an Unreal Engine 5 plugin that allows you to automatically generate a controller class for a given Widget Blueprint from within the Unreal Editor. It also provides the ability to update those h/cpp files automatically from within the editor when adding more widgets. The header files are automatically found and included as well.

To use it:
	1) Copy/paste the repo into the Plugins directory of an Unreal Engine 5 project.
	2) Build/run the project (in an Editor configuration)
	3) Enable the plugin in the Plugins menu.
	4) Right click a Widget Blueprint->Scripted Asset Actions->Create Wbp Controller.
	
While I expect this to work with other versions as well, this has only been tested so far with Unreal 5.1.