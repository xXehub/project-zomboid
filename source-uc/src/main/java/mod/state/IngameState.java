package mod.state;

import mod.character.IsoGameCharacter;
import mod.inventory.InventoryItem;
import mod.lua.LuaCustom;
import mod.trait.CharacterTraitDefinition;
import net.bytebuddy.asm.Advice;
import zombie.GameSounds;
import zombie.SystemDisabler;
import zombie.audio.GameSound;

public class IngameState {

	@Advice.OnMethodExit
	public static void enter() {
		System.out.println("IN GAME STATE");
		if (zombie.Lua.LuaManager.exposer != null) {
            zombie.Lua.LuaManager.exposer.exposeGlobalFunctions(new LuaCustom());
            System.out.println("[INFO] LuaCustom global functions registered.");
            SystemDisabler.setEnableAdvancedSoundOptions(true);
        	GameSound sound = GameSounds.getSound("HeartBeat");
        	sound.setUserVolume(0);
        }
		
	}
}
