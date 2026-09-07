package mod.weapon;

import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.inventory.InventoryItem;
import mod.debug.Debug;
import net.bytebuddy.asm.Advice;

public class HandWeapon {
	
	public static boolean x2 = false;
	public static boolean infinityammo = true;
	
	@RuntimeType
	public static float getCriticalChance() {
		return 1000f;
	}
	
	@RuntimeType
	public static int getHitChance() {
		return 1000;
	}
	
	@RuntimeType
	public static int getAimingTime() {
		return 0;
	}
	
	
	public static class ReloadTime{
		@Advice.OnMethodExit
		public static void getReloadTime(@Advice.Return(readOnly = false) int ogValue) {
			System.out.println("Reload Time .. " + ogValue);
			ogValue = ogValue / 2;
		}
	}
	
	public static class GetRecoilDelay{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) int ogValue) {
			ogValue = ogValue / 2;
		}
	}
	
	public static class GetDamageMod{
		@Advice.OnMethodExit
		public static void run(@Advice.This Object obj, @Advice.Return(readOnly = false) float ogValue) {
			zombie.inventory.types.HandWeapon handWeapon = (zombie.inventory.types.HandWeapon) obj;
			if(handWeapon.isRanged()) {
//				Debug.Log("RangedWeapon");
				ogValue = 3.0f;
			}else {
//				Debug.Log("MeleeWeapon");
				if(x2) {
					ogValue = 2.0f;
				}else {
					ogValue = 1.0f;
				}
				
				
			}
			
		}
	}
	
	public static class GetRangedMod{
		@Advice.OnMethodExit
		public static void run(@Advice.This Object obj, @Advice.Return(readOnly = false) float ogValue) {
			zombie.inventory.types.HandWeapon handWeapon = (zombie.inventory.types.HandWeapon) obj;
			if(handWeapon.isRanged()) {
//				Debug.Log("RangedWeapon");
				ogValue = 3.0f;
			}else {
//				Debug.Log("MeleeWeapon");
				ogValue = 1.0f;
			}
			
		}
	}
		
	
	public static class MaxRange{
		@Advice.OnMethodExit
		public static void getMaxRange(@Advice.This Object obj, @Advice.Return(readOnly = false) float ogValue) {
			zombie.inventory.types.HandWeapon handWeapon = (zombie.inventory.types.HandWeapon) obj;
			if(handWeapon.isRanged()) {
				ogValue = ogValue + (ogValue * 0.5f);
			}
			
		}
	}

	public static class IsRoundChambered{
		@Advice.OnMethodExit
		public static void run(@Advice.This Object obj, @Advice.Return(readOnly = false) boolean round) {
			zombie.inventory.types.HandWeapon handWeapon = (zombie.inventory.types.HandWeapon) obj;
			if(infinityammo) {
				if(handWeapon.getCurrentAmmoCount() == 1) {
					round = true;
				}				
			}
		}
	}
	
	
}
