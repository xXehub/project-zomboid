package mod.inventory;

import net.bytebuddy.asm.Advice;
import zombie.characters.IsoPlayer;

public class Food {
	
	@Advice.OnMethodExit
	public static void getName(@Advice.Argument(0) IsoPlayer player, @Advice.FieldValue(value = "container", readOnly = false) zombie.inventory.ItemContainer container,
			@Advice.FieldValue(value = "age", readOnly = false) int age) {
		if(container.getParent() instanceof IsoPlayer){
			if(container.getParent().equals(IsoPlayer.getInstance())) {
				age = 0;
			}
		}
	}
}
