package mod.inventory;

import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.characters.IsoPlayer;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.scripting.objects.ItemKey.Key;

public class ItemContainer {

	@RuntimeType
	public static int getCapacity() {
		return 80;
	}

	@RuntimeType
	public static int getWeightReduction() {
		return 1000;
	}

	@RuntimeType
	public static float getMaxWeight() {
		return 80;
	}

	@RuntimeType
	public static float getCapacityWeight() {
		return 0.0f;
	}

	@Advice.OnMethodExit
	public static void haveThisKeyId() {
		System.out.println("Vehicle container accessed");
	}

}
