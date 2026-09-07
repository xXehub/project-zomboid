package mod.lua;

import java.awt.KeyboardFocusManager;
import java.io.File;

import org.lwjglx.input.Keyboard;

import mod.Agent;
import mod.character.IsoGameCharacter;
import mod.console.DevConsole;
import mod.debug.Debug;
import mod.esp.Esp;
import mod.vehicle.Vehicle;
import net.bytebuddy.asm.Advice;
import se.krka.kahlua.vm.KahluaTable;
import zombie.SandboxOptions;
import zombie.characters.IsoPlayer;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.inventory.types.HandWeapon;
import zombie.scripting.objects.WeaponReloadType;

public class LuaEventManager {

	public static boolean toggleNightVision = false;

	public static class TriggerEvent{
		@Advice.OnMethodEnter
		public static void triggerEvent(@Advice.Argument(0) String event, @Advice.Argument(1) Object param1) {
			if(event.equals("OnKeyPressed")) {

				if((int)param1 == Keyboard.KEY_F3) {
					InventoryItem item = InventoryItemFactory.CreateItem("Base.AssaultRifle");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					InventoryItem clip = InventoryItemFactory.CreateItem("Base.556Clip");
					clip.setCurrentAmmoCount(clip.getMaxAmmo());
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(clip, 0.0F, 0.0F, 0.0F);
				}
				if((int)param1 == Keyboard.KEY_F4) {
					if(IsoPlayer.getInstance().getPrimaryHandItem() != null) {
						if(IsoPlayer.getInstance().getPrimaryHandItem().IsWeapon()) {
							HandWeapon weapon = (HandWeapon)IsoPlayer.getInstance().getPrimaryHandItem();
							InventoryItem clip = InventoryItemFactory.CreateItem(weapon.getMagazineType());
							clip.setCurrentAmmoCount(clip.getMaxAmmo());
							IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(clip, 0.0F, 0.0F, 0.0F);
						}
					}
				}
				if((int)param1 == Keyboard.KEY_F5) {
					System.setProperty("java.awt.headless", "false");
				    System.out.println("Opening console. Headless: " + System.getProperty("java.awt.headless"));
					DevConsole.toggle();
					if (zombie.Lua.LuaManager.exposer != null) {
			            zombie.Lua.LuaManager.exposer.exposeGlobalFunctions(new LuaCustom());
			            System.out.println("[INFO] LuaCustom global functions registered.");
			        }
				}
				if((int)param1 == Keyboard.KEY_F6) {
					
					try {
						File agentJar = new File(Agent.class.getProtectionDomain().getCodeSource().getLocation().toURI());
						Debug.Log(agentJar.getAbsolutePath());
						Debug.Log(agentJar.getParent());
					} catch (Exception e) {
						Debug.Log(e.getMessage());
					}
					
					
					
//					SandboxOptions.instance.characterFreePoints.setValue(200);
//					KahluaTable vars = (KahluaTable) zombie.Lua.LuaManager.env.rawget("SandboxVars");
//					if (vars != null) {
//					    vars.rawset("CharacterFreePoints", 200.0);
//					    System.out.println("Free points modified successfully in Lua!");
//					}
				}
				if((int)param1 == Keyboard.KEY_F7) {
					mod.weapon.HandWeapon.infinityammo = !mod.weapon.HandWeapon.infinityammo;
				}
				if((int)param1 == Keyboard.KEY_F9) {
					Vehicle.active = !Vehicle.active;
					Debug.Log("Vehicle Active hack: " + ((Vehicle.infinityTank)? "Enabled" : "Disabled"));
				}
				if((int)param1 == Keyboard.KEY_F8) {
					Esp.enabled = !Esp.enabled;
					Debug.Log("Esp : " + ((Esp.enabled)? "Enabled" : "Disabled"));
				}

				if((int)param1 == Keyboard.KEY_F10) {
					Vehicle.tryStartEngine();
					Debug.Log("Trying to Start Vehicle");
				}
				if((int)param1 == Keyboard.KEY_F11) {
					Debug.Log("Night Vision : " + IsoGameCharacter.nightVision);
					IsoGameCharacter.nightVisionToggle();
				}

				if((int)param1 == Keyboard.KEY_RSHIFT) {
					if(IsoPlayer.getInstance().getVehicle() != null) {
						if(Vehicle.enginePower != 15000) {
							Vehicle.enginePower = 15000;
						}else {
							Vehicle.enginePower = 300;
						}
					}
				}

			}
		}
	}


}
