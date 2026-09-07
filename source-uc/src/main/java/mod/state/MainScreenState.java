package mod.state;

import mod.character.IsoGameCharacter;
import mod.inventory.InventoryItem;
import mod.trait.CharacterTraitDefinition;
import net.bytebuddy.asm.Advice;

public class MainScreenState {

	@Advice.OnMethodExit
	public static void enter() {
		System.out.println("Enter to main screen");
		IsoGameCharacter.once = false;
		CharacterTraitDefinition.finish = false;
	}
}
