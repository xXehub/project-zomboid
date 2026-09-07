package mod.lua;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import mod.debug.Debug;

import se.krka.kahlua.integration.annotations.LuaMethod;
import se.krka.kahlua.vm.KahluaTable;
import zombie.characters.IsoPlayer;
import zombie.entity.components.crafting.recipe.CraftRecipeData;
import zombie.entity.components.crafting.recipe.CraftRecipeData.InputScriptData;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.inventory.types.DrainableComboItem;
import zombie.iso.IsoGridSquare;
import zombie.iso.IsoObject;
import zombie.iso.objects.IsoDoor;
import zombie.iso.objects.IsoThumpable;
import zombie.iso.objects.IsoWorldInventoryObject;
import zombie.iso.sprite.IsoSprite;
import zombie.iso.sprite.IsoSpriteGrid;
import zombie.scripting.entity.components.crafting.CraftRecipe;
import zombie.scripting.entity.components.crafting.InputScript;
import zombie.scripting.objects.ItemKey.Key;
import zombie.vehicles.BaseVehicle;

public class LuaCustom {


	@LuaMethod(name = "ToggleDoor", global = true)
    public static Boolean ToggleDoor(IsoDoor door) {
		Debug.Log("ToggleDoor ");
		door.setOpen(!door.isOpen());

		return true;
	}

	@LuaMethod(name = "Destroy", global = true)
    public static Boolean Destroy(IsoDoor door) {
		Debug.Log("In Destroy");
		if (door.destroyDoubleDoor(door)) {
		}else if (door.destroyGarageDoor(door)) {
		}else {
			door.destroy();
		}
		Debug.Log("Destroy End");

		return true;
	}

	@LuaMethod(name = "DestroyObject", global = true)
    public static Boolean DestroyObject(IsoObject obj) {
		Debug.Log("In Destroy");
		IsoSprite sprite = obj.getSprite();

		if(sprite != null && sprite.getSpriteGrid() != null) {
			IsoSpriteGrid grid = sprite.getSpriteGrid();

			int gridx = grid.getSpriteGridPosX(sprite);
			int gridy = grid.getSpriteGridPosY(sprite);

			IsoGridSquare baseGrid = obj.getSquare();

			IsoGridSquare newSquare = IsoPlayer.getInstance().getSquare();

			int deltaX = newSquare.getX() - baseGrid.getX();
			int deltaY = newSquare.getY() - baseGrid.getY();
			int deltaZ = newSquare.getZ() - baseGrid.getZ();

			List<IsoObject> objToMove = new ArrayList<IsoObject>();

			for(int x = 0; x < grid.getWidth(); x++) {
				for(int y = 0; y < grid.getHeight(); y++) {

					int searchX = baseGrid.getX() - gridx + x;
	                int searchY = baseGrid.getY() - gridy + y;

	                IsoGridSquare nextGrid = IsoPlayer.getInstance().getCell().getGridSquare(searchX, searchY, baseGrid.getZ());

	                if (nextGrid != null) {
	                    IsoObject bro = findPieceByGrid(nextGrid, grid);
	                    if (bro != null) {
	                    	objToMove.add(bro);
	                    }
	                }
				}
			}

			for (IsoObject bro : objToMove) {
	            IsoGridSquare oldGrid = bro.getSquare();
	            oldGrid.transmitRemoveItemFromSquare(bro);
            	oldGrid.RemoveTileObject(bro);
            	obj.removeFromSquare();
	        }

		}else {
			obj.getSquare().transmitRemoveItemFromSquare(obj);
			obj.removeFromSquare();
			obj.removeFromWorld();
			obj.getSquare().RecalcAllWithNeighbours(true);
		}

		Debug.Log("Destroy End");

		return true;
	}

	@LuaMethod(name = "CreateKeyVehicle", global = true)
    public static Boolean CreateKeyVehicle(BaseVehicle vehicle) {
		InventoryItem item = InventoryItemFactory.CreateItem(Key.CAR_KEY);
		item.setKeyId(vehicle.getKeyId());
		vehicle.keyNamerVehicle(item);
		IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);

		return true;
	}

	@LuaMethod(name = "GodProvides", global = true)
    public static Boolean GodProvides(String baseType) {

		Debug.Log("Creating Item : "  + baseType);
		InventoryItem item = InventoryItemFactory.CreateItem(baseType);
		IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);

		return true;
	}


	@LuaMethod(name = "HasRequiredObject", global = true)
	public static int HasRequiredObject(String type, int amount) {

		int amountCount = 0;


		ArrayList<IsoWorldInventoryObject> list = (ArrayList<IsoWorldInventoryObject>) IsoPlayer.getInstance().getSquare().getWorldObjects().stream()
		.filter(x -> x.getItem().getFullType().equals(type))
		.collect(Collectors.toList());

		for(IsoWorldInventoryObject obj : list) {
			if(obj.getItem() instanceof DrainableComboItem) {
				Debug.Log("HasRequiredObject Drainable");
				amountCount += obj.getItem().getCurrentUses();
			}else {
				amountCount += 1;
			}
		}


		ArrayList<InventoryItem> playerInvItems = (ArrayList<InventoryItem>) IsoPlayer.getInstance().getInventory().getItems().stream()
		.filter(x -> x.getFullType().equals(type))
		.collect(Collectors.toList());


		for(InventoryItem item : playerInvItems) {

			if(item instanceof DrainableComboItem)  {
				Debug.Log("HasRequiredObject Drainable");
				amountCount += item.getCurrentUses();
			}else {
				amountCount += 1;
			}
		}

		return amountCount;
	}

	public static IsoObject findPieceByGrid(IsoGridSquare square, IsoSpriteGrid targetGrid) {
	    for (int i = 0; i < square.getObjects().size(); i++) {
	        IsoObject obj = square.getObjects().get(i);
	        if (obj.getSprite() != null && obj.getSprite().getSpriteGrid() == targetGrid) {
	            return obj;
	        }
	    }
	    return null;
	}

	@LuaMethod(name = "MoveObject", global = true)
	public static Boolean MoveObject(IsoObject obj) {

		IsoSprite sprite = obj.getSprite();

		if(sprite != null && sprite.getSpriteGrid() != null) {
			IsoSpriteGrid grid = sprite.getSpriteGrid();

			int gridx = grid.getSpriteGridPosX(sprite);
			int gridy = grid.getSpriteGridPosY(sprite);

			IsoGridSquare baseGrid = obj.getSquare();

			IsoGridSquare newSquare = IsoPlayer.getInstance().getSquare();

			int deltaX = newSquare.getX() - baseGrid.getX();
			int deltaY = newSquare.getY() - baseGrid.getY();
			int deltaZ = newSquare.getZ() - baseGrid.getZ();

			List<IsoObject> objToMove = new ArrayList<IsoObject>();

			for(int x = 0; x < grid.getWidth(); x++) {
				for(int y = 0; y < grid.getHeight(); y++) {

					int searchX = baseGrid.getX() - gridx + x;
	                int searchY = baseGrid.getY() - gridy + y;

	                IsoGridSquare nextGrid = IsoPlayer.getInstance().getCell().getGridSquare(searchX, searchY, baseGrid.getZ());

	                if (nextGrid != null) {
	                    IsoObject bro = findPieceByGrid(nextGrid, grid);
	                    if (bro != null) {
	                    	objToMove.add(bro);
	                    }
	                }
				}
			}

			for (IsoObject bro : objToMove) {
	            IsoGridSquare oldGrid = bro.getSquare();

	            int destX = oldGrid.getX() + deltaX;
	            int destY = oldGrid.getY() + deltaY;
	            int destZ = oldGrid.getZ() + deltaZ;

	            IsoGridSquare destSquare = IsoPlayer.getInstance().getCell().getGridSquare(destX, destY, destZ);

	            if (destSquare != null) {
	            	oldGrid.transmitRemoveItemFromSquare(bro);
	            	oldGrid.RemoveTileObject(bro);
	            	obj.removeFromSquare();
	                bro.setSquare(destSquare);

	                obj.getSquare().transmitAddObjectToSquare(obj, -1);
	                destSquare.AddTileObject(bro);
	                oldGrid.RecalcAllWithNeighbours(true);
	                destSquare.RecalcAllWithNeighbours(true);
	            }
	        }

		}else {
			obj.getSquare().transmitRemoveItemFromSquare(obj);
			obj.removeFromSquare();
			obj.removeFromWorld();
			obj.getSquare().RecalcAllWithNeighbours(true);


			obj.setSquare(IsoPlayer.getInstance().getSquare());

			obj.getSquare().transmitAddObjectToSquare(obj, -1);
			obj.getSquare().AddTileObject(obj);
			obj.getSquare().RecalcAllWithNeighbours(true);
		}

		return true;
	}

	@LuaMethod(name = "DestroyVehicle", global = true)
    public static Boolean DestroyVehicle(BaseVehicle vehicle) {
		vehicle.getSquare().transmitRemoveItemFromSquare(vehicle);
		vehicle.removeFromWorld();
		vehicle.permanentlyRemove();

		return true;
	}


}
