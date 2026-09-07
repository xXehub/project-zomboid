package mod.inventory;

import java.util.List;

import mod.debug.Debug;
import net.bytebuddy.asm.Advice;
import zombie.characters.IsoPlayer;
import zombie.entity.components.crafting.recipe.HandcraftLogic;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.scripting.entity.components.crafting.InputScript;
import zombie.scripting.objects.Item;

public class HandCraftLogic {

	public static class performCurrentRecipe{
		@Advice.OnMethodEnter
		public static void run(@Advice.This Object logic) {
			HandcraftLogic hlogic = (HandcraftLogic) logic;
			
			for (InputScript input : hlogic.getRecipe().getInputs()) {
				float amount = input.getAmount();
				
				List<Item> inputObjects = hlogic.getSatisfiedInputItems(input);
				
				if(inputObjects.size() == 0) {
					inputObjects = input.getPossibleInputItems();
				}
				
				String fullName = inputObjects.get(0).getFullName();
				
				Debug.Log("Creating Item : "  + fullName);
				InventoryItem item = InventoryItemFactory.CreateItem(fullName);
				IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
			}
		}
	}
}
