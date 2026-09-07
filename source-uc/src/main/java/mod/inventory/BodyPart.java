package mod.inventory;

import java.lang.reflect.Field;

import net.bytebuddy.asm.Advice.FieldValue;
import net.bytebuddy.implementation.bind.annotation.Argument;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import net.bytebuddy.implementation.bind.annotation.This;
import zombie.characters.IsoGameCharacter;
import zombie.characters.IsoPlayer;
import zombie.core.math.PZMath;

public class BodyPart {
	
	@RuntimeType
	public static void ReduceHealth(@Argument(0) float val, @This Object bodyPart, @FieldValue(value="parentChar") IsoGameCharacter parentChar) {
		try {
			
			if(parentChar instanceof IsoPlayer) {
				if(IsoPlayer.getInstance().equals(parentChar)) {
					if(IsoPlayer.getInstance().getVehicle() != null) {
						return;
					}
				}
			}
			
			Field fractureTimeVar = bodyPart.getClass().getDeclaredField("fractureTime");
			       
			fractureTimeVar.setAccessible(true);
			float fractureTime = fractureTimeVar.getFloat(bodyPart);
	                
			if(fractureTime <= 0.0f) {
				Field healthVar = bodyPart.getClass().getDeclaredField("health");        
				healthVar.setAccessible(true);
				healthVar.set(bodyPart, PZMath.clamp(healthVar.getFloat(bodyPart) - val, 0.0F, 100.0F));
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
		
	}
	
	
	
	
	
}
