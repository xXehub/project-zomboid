package mod.lua;

import java.io.File;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;

import mod.Agent;
import mod.debug.Debug;
import net.bytebuddy.asm.Advice;
import zombie.core.Core;
import zombie.network.GameClient;

public class LuaManager {

	public static String replacedPath = Agent.mainPath + "/moddata/lua/replaced";
	public static String newPath = Agent.mainPath + "/moddata/lua/new";

	public static boolean automaticlua = true;


	public static class FinishCheckSum{
		@Advice.OnMethodExit
		public static void run() {
			if(automaticlua) {

				Debug.Log("FinishCheckSum Intercept");
				if(zombie.Lua.LuaManager.checksumDone || (!GameClient.client)) {

					LinkedHashMap<String, String> namesAndPath = getFiles(replacedPath);
					LinkedHashMap<String, String> namesAndPathNew = getFiles(newPath);

					ArrayList<String> loadedCpy = new ArrayList<>(zombie.Lua.LuaManager.loaded);


					Debug.Log("FinishCheckSum Was Successfull");
					for(String filename: zombie.Lua.LuaManager.loaded) {
						Debug.Log("LuaManager Loaded: " + filename);
					}

					for(var entry : namesAndPath.entrySet()) {
						for(String filename : loadedCpy) {
							if(filename.contains(entry.getKey())) {
								Debug.Log("Lua Filename to replace finded: " + filename);
								zombie.Lua.LuaManager.loaded.remove(filename);
								Debug.Log("Lua Filename removed.");
								zombie.Lua.LuaManager.RunLua(entry.getValue(), true);
								Debug.Log("Lua Filename " + entry.getKey() + " Executed.");
							}
						}
					}

					for(var entry : namesAndPathNew.entrySet()) {
						if(!loadedCpy.contains(entry.getValue())) {
							zombie.Lua.LuaManager.RunLua(entry.getValue(), true);
							Debug.Log("Lua Filename " + entry.getKey() + " Executed.");
						}else {
							zombie.Lua.LuaManager.loaded.remove(entry.getValue());
						}
					}

					Debug.Log("Finish Replace Lua Files");
					for(String filename: zombie.Lua.LuaManager.loaded) {
						Debug.Log("LuaManager Loaded: " + filename);
					}
				}
			}
		}

		public static LinkedHashMap<String, String> getFiles(String path){

			LinkedHashMap<String, String> namesAndPath = new LinkedHashMap<String, String>();

			File folder = new File(path);

	        if (folder.exists() && folder.isDirectory()) {
	            File[] files = folder.listFiles();

	            if (files != null) {
	                for (File file : files) {
	                    if (file.isFile()) {
	                    	namesAndPath.put(file.getName(), file.getAbsolutePath());
	                    }
	                }
	            } else {
	                System.out.println("Directory is empty or cannot be read.");
	            }
	        } else {
	            System.out.println("The specified path does not exist or is not a valid directory.");
	        }
	        return namesAndPath;
		}
	}




}
