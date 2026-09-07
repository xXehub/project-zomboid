package mod.inventory;

import net.bytebuddy.asm.Advice.Argument;
import net.bytebuddy.asm.Advice.FieldValue;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.characters.IsoGameCharacter;
import zombie.characters.IsoPlayer;

public class BodyDamage {
	
	@RuntimeType
	public static void setOverallBodyHealth(
			@FieldValue(value="parentChar", readOnly = false) IsoGameCharacter parentChar,
			@FieldValue(value="overallBodyHealth", readOnly = false) float overallBodyHealth,
			@Argument(0) float health) {
		
		if(parentChar instanceof IsoPlayer) {
			if(IsoPlayer.getInstance().equals(parentChar)) {
				if(IsoPlayer.getInstance().getVehicle() != null) {
					if(overallBodyHealth < health) {
						return;
					}					
				}	
			}			
		}
		
		overallBodyHealth = health;						
	}
}
