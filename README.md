# UmgControllerGeneratorPlugin
This is an Unreal Engine 5 plugin that allows you to automatically generate a controller class for a given Widget Blueprint from within the Unreal Editor. It also provides the ability to update those h/cpp files automatically from within the editor when adding more widgets. The header files are automatically found and included as well.

To use the plugin:

	1) Copy/paste the repo into the Plugins directory of an Unreal Engine 5 project.
	
	2) Build/run the project (in an Editor configuration)
	
	3) Enable the plugin in the Plugins menu.
	
	4) Right click a Widget Blueprint->Scripted Asset Actions->WBP Create Controller.
	
You can update an existing WBP's controller that has been modified as well:
	
	- Right click a Widget Blueprint->Scripted Asset Actions->WBP Update Controller.

If you rename or move a Widget Blueprint, you can update this plugin's mapping to its source files:

	- Right click a Widget Blueprint->Scripted Asset Actions->Update Mappings.
	
You can configure the plugin settings by placing the following in your Config/DefaultEditor.ini (in your project, not in the plugin):

```
[/Script/UmgControllerGeneratorPlugin.CodeGeneratorConfig]
ClassSuffix="Controller"
BlueprintSourceMapDirectory=""
EnableAutoReparenting=true
```
	
While I expect this to work with other versions as well, this has only been tested so far with Unreal 5.1.

Notes:

	- Your C++ source files need to be named the same as your Widget Blueprints (plus the configurable ClassSuffix and minus the WBP_ prefix) in order for UpdateMappings to work. For example, if your Widget Blueprint is named "WBP_Menu" and the suffix is "Controller", your h/cpp files need to be named MenuController with a class called UMenuController inside.
	
	- The BlueprintSourceMap.json file should be source controlled so other team members can update as well. The paths are relative to the project's source directory. The path to the directory this file is in can be configured using BlueprintSourceMapDirectory. This path is relative to the project directory and defaults to the root.
