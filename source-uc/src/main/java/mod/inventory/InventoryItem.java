package mod.inventory;

import mod.character.IsoGameCharacter;
import mod.trait.CharacterTraitDefinition;
import mod.weapon.HandWeapon;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;

public class InventoryItem {
	
	public static boolean once = false;
	
	@RuntimeType
    public static float getWeight() {
		
		if(!once) {
			CharacterTraitDefinition.getCharacterTraitsByReflection();
			IsoGameCharacter.general();
			once = true;
		}
		
        return 0.01f;
    }
	
	@RuntimeType
    public static float getActualWeight() {
        return 0.01f;
    }
	
	@RuntimeType
	public static float getContentsWeight() {
		return 0.01f;
	}
	
	@RuntimeType
	public static float getHotbarEquippedWeight() {
		return 0.01f;
	}
	
	@RuntimeType
	public static float getEquippedWeight() {
		return 0.01f;
	}
	
	@RuntimeType
	public static float getUnequippedWeight() {
		return 0.01f;
	}
		
	
	public static class Sharpness{
		
		@Advice.OnMethodExit
		public static void getSharpness(@Advice.Return(readOnly = false) float ogValue) {
			if(ogValue > 0.0f) {
				ogValue = ogValue * 1.3f;
			}
		}	
	}
	
	public static class GetCurrentAmmoCount{
		
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) int currentAmmo) {
			if(HandWeapon.infinityammo) {
				if(currentAmmo == 0) {
					currentAmmo = 1;
				}				
			}
		}	
	}	
		
}
