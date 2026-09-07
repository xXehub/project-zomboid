package mod.console;

import javax.swing.*;

import mod.character.IsoGameCharacter;
import mod.debug.Debug;
import mod.esp.Esp;
import mod.inventory.Food;
import mod.lua.LuaManager;
import mod.radar.Radar;
import mod.trait.CharacterTraitDefinition;
import mod.vehicle.Vehicle;
import mod.weapon.HandWeapon;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.util.ArrayList;

import zombie.GameSounds;
import zombie.SystemDisabler;
import zombie.audio.GameSound;
import zombie.characters.IsoPlayer;
import zombie.characters.animals.AnimalDefinitions;
import zombie.characters.animals.datas.AnimalBreed;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.inventory.ItemContainer;
import zombie.inventory.types.InventoryContainer;
import zombie.iso.objects.IsoWorldInventoryObject;
import zombie.scripting.objects.ItemKey.Key;
import zombie.worldMap.UIWorldMap;

public class DevConsole {
    private static JFrame frame;
    private static JTextField inputField;
    private static JTextArea logArea;

    public static void toggle() {
        SwingUtilities.invokeLater(() -> {
            if (frame == null) {
                initUI();
            }
            frame.setVisible(!frame.isVisible());

            if (frame.isVisible()) {
                frame.toFront();
                inputField.requestFocus();
            }
        });
    }

    private static void initUI() {
        frame = new JFrame("PZ Dev Console");
        frame.setSize(600, 400);
        frame.setLayout(new BorderLayout());
        frame.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);

        Font consoleFont = new Font("Monospaced", Font.PLAIN, 14);

        logArea = new JTextArea();
        logArea.setEditable(false);
        logArea.setBackground(Color.BLACK);
        logArea.setForeground(Color.GREEN);
        logArea.setFont(consoleFont);

        JScrollPane scrollPane = new JScrollPane(logArea);
        scrollPane.setBorder(null);
        frame.add(scrollPane, BorderLayout.CENTER);

        inputField = new JTextField();
        inputField.setBackground(Color.DARK_GRAY);
        inputField.setForeground(Color.WHITE);
        inputField.setCaretColor(Color.WHITE);
        inputField.setFont(consoleFont);
        inputField.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

        inputField.addActionListener((ActionEvent e) -> {
            String input = inputField.getText().trim();
            if (!input.isEmpty()) {
                print("> " + input);
                executeCommand(input);
                inputField.setText("");
            }
        });

        frame.add(inputField, BorderLayout.SOUTH);
        frame.add(new RadarPanel(), BorderLayout.EAST);
        frame.setLocationRelativeTo(null);

        print("Dev Console.");
    }

    public static void print(String message) {
        if (logArea != null) {
            logArea.append(message + "\n");
            logArea.setCaretPosition(logArea.getDocument().getLength());
        }
    }

    private static void executeCommand(String input) {
        String[] args = input.split(" ");
        String command = args[0].toLowerCase();

        try {
            IsoPlayer player = IsoPlayer.getInstance();

            switch (command) {
                case "help":
                    print("Commands: \n clear, key, repaircar, traits, reloadlua, repairhotwire");
                    break;
                case "clear":
                    logArea.setText("");
                    break;
                case "key":
                	if(IsoPlayer.getInstance().getVehicle() != null) {
                		if(IsoPlayer.getInstance().getInventory().haveThisKeyId(IsoPlayer.getInstance().getVehicle().getKeyId()) == null) {
                			InventoryItem item = InventoryItemFactory.CreateItem(Key.CAR_KEY);
                			item.setKeyId(IsoPlayer.getInstance().getVehicle().getKeyId());
                			IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
                			print("Key was created successfully");
                		}else {
                			print("You already have the key");
                		}
                	}else {
                		print("Player is not in vehicle");
                	}
                	break;
                case "repaircar":
                	if(IsoPlayer.getInstance().getVehicle() != null) {
                		IsoPlayer.getInstance().getVehicle().repair();
                	}
                	break;
                case "traits":
                	CharacterTraitDefinition.getCharacterTraitsByReflection();
                	print("traits added!");
                	break;
                case "reloadlua":
                	LuaManager.FinishCheckSum.run();
                	print("Files has been reloaded!");
                	break;
                case "repairhotwire":
                	Vehicle.repairhotwire = !Vehicle.repairhotwire;
                	print("Repair hotwire broken is " + ((Vehicle.repairhotwire)? "Enabled" : "Disabled"));
                	break;
                case "audio":
                	SystemDisabler.setEnableAdvancedSoundOptions(true);
                	GameSound sound = GameSounds.getSound("HeartBeat");
                	sound.setUserVolume(0);
                	break;
                case "esp":
                	Esp.enabled = !Esp.enabled;
                	print("Esp is " + ((Esp.enabled)? "Enabled" : "Disabled"));
                	break;
                case "x2":
                	HandWeapon.x2 = !HandWeapon.x2;
                	print("X2 " + ((HandWeapon.x2)? "Enabled" : "Disabled"));
                	break;
                case "artisan":
                	IsoGameCharacter.artisan = !IsoGameCharacter.artisan;
                	print("Artisan: " + ((IsoGameCharacter.artisan)? "Enabled" : "Disabled"));
                	break;
                case "automaticlua":
                	LuaManager.automaticlua = !LuaManager.automaticlua;
                	print("Automatic Lua Injection : " + ((LuaManager.automaticlua)? "Enabled" : "Disabled"));
                	break;
                case "axe":
                	{
                		InventoryItem item = InventoryItemFactory.CreateItem("Base.WoodAxe");
    					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
                	}
                	break;
                case "plank":
                	{
                		for(int i = 0; i < 20; i++) {
                			InventoryItem item = InventoryItemFactory.CreateItem("Base.Plank");
        					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
                		}
                	}
                	break;
                case "log":
	            	{
	            		for(int i = 0; i < 10; i++) {
	            			InventoryItem item = InventoryItemFactory.CreateItem("Base.Log");
	    					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            		}
	            	}
	            	break;
                case "katana":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.Katana");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
	            	break;
                case "nails":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.NailsBox");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
            	break;
                case "food":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.TurkeyWhole");
	            		item.cooked = true;
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
            	break;
                case "generator":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.Generator_Yellow");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
	        	break;
                case "welderkit":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.BlowTorch");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            		item = InventoryItemFactory.CreateItem("Base.PropaneTank");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
						item = InventoryItemFactory.CreateItem("Base.WeldingMask");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
						item = InventoryItemFactory.CreateItem("Base.WeldingRods");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
            	break;
                case "propane":
	            	{
	            		for(int i = 0; i < 5; i++) {
	            			InventoryItem item = InventoryItemFactory.CreateItem("Base.PropaneTank");
							IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            		}
	            	}
	            break;
                case "weldingrods":
            	{
            		for(int i = 0; i < 5; i++) {
            			InventoryItem item = InventoryItemFactory.CreateItem("Base.WeldingRods");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            		}
            	}
            	break;

                case "sheet":
            	{
            		for(int i = 0; i < 10; i++) {
            			InventoryItem item = InventoryItemFactory.CreateItem("Base.SheetMetal");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            		}
            	}
            	break;
                case "rod":
                	{
                		for(int i = 0; i < 10; i++) {
                			InventoryItem item = InventoryItemFactory.CreateItem("Base.MetalBar");
    						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
                		}
	            	}
                break;
                case "pipe":
				{
					for(int i = 0; i < 10; i++) {
						InventoryItem item = InventoryItemFactory.CreateItem("Base.MetalPipe");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					}
				}
				break;
                case "brick":
	            	{
	            		for(int i = 0; i < 10; i++) {
	            			InventoryItem item = InventoryItemFactory.CreateItem("Base.ClayBrick");
							IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            		}
	            	}
            	break;
                case "stoneblock":
	            	{
	            		for(int i = 0; i < 10; i++) {
	            			InventoryItem item = InventoryItemFactory.CreateItem("Base.StoneBlock");
							IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            		}
	            	}
            	break;
                case "stone":
            	{
            		for(int i = 0; i < 10; i++) {
            			InventoryItem item = InventoryItemFactory.CreateItem("Base.Stone2");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            		}
            	}
            	break;
                case "cement":
            	{
            		for(int i = 0; i < 5; i++) {
            			InventoryItem item = InventoryItemFactory.CreateItem("Base.BucketConcreteFull");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            		}
            	}
            	break;
                case "masonrykit":
            	{
            		InventoryItem item = InventoryItemFactory.CreateItem("Base.MasonsTrowel");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.MasonsChisel");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.Hammer");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            	}
            	break;
                case "weldingmagazines":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.MetalworkMag1");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
						item = InventoryItemFactory.CreateItem("Base.MetalworkMag2");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
						item = InventoryItemFactory.CreateItem("Base.MetalworkMag3");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
						item = InventoryItemFactory.CreateItem("Base.MetalworkMag4");
						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
            	break;
                case "weldingbooks":
            	{
            		InventoryItem item = InventoryItemFactory.CreateItem("Base.BookMetalWelding1");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.BookMetalWelding2");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.BookMetalWelding3");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.BookMetalWelding4");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
					item = InventoryItemFactory.CreateItem("Base.BookMetalWelding5");
					IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
            	}
        	break;
                case "animals":
                	{
                		ArrayList<AnimalDefinitions> defs = AnimalDefinitions.getAnimalDefsArray();
                		for(AnimalDefinitions def : AnimalDefinitions.getAnimalDefsArray()) {
                			Debug.Log(def.getAnimalType());
                			for(AnimalBreed breed : def.getBreeds()){
                				Debug.Log(breed.getName());
                			}
                		}
                	}
                break;
                case "medicalkit":
	            	{
	            		InventoryItem item = InventoryItemFactory.CreateItem("Base.Bag_MedicalBag");

	            		InventoryContainer ic = (InventoryContainer) item;

	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.AdhesiveBandageBox"));

	            		for(int i = 0; i < 4; i++) {
	            			ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.AlcoholWipes"));
	            		}

	            		for(int i = 0; i < 10; i++) {
	            			ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.SutureNeedle"));
	            		}

	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.Thread"));
	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.Thread"));
	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.SutureNeedleHolder"));
	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.Tweezers"));
	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.Pills"));
	            		ic.getInventory().addItem(InventoryItemFactory.CreateItem("Base.Pills"));

						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	            	}
	            	break;
                case "clearsquare":
	                {
	                	try {
	                		IsoPlayer.getInstance().getSquare().getObjects().stream()
	                		.filter(x -> x instanceof IsoWorldInventoryObject).forEach(obj -> {
		                		obj.getSquare().transmitRemoveItemFromSquare(obj);
			            		obj.removeFromSquare();
			            		obj.removeFromWorld();
		                	});
						} catch (Exception e) {
							Debug.Log(e.getMessage());
						}
	                }
	                break;
                default:
                	if(args[0].startsWith("Base.")) {
                		try {
                			{
        	            		InventoryItem item = InventoryItemFactory.CreateItem(args[0]);
        						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
        	            	}
						} catch (Exception e) {
						}
                	}else if(args[0].startsWith("Create")){
                		{
                			try {
	                    		InventoryItem item = InventoryItemFactory.CreateItem(args[1]);
	    						IsoPlayer.getInstance().getSquare().AddWorldInventoryItem(item, 0.0F, 0.0F, 0.0F);
	                		} catch (Exception e) {
							}
                		}
                	}else {
                		print("Unknown command: " + command);
                	}
                break;
            }
        } catch (Exception ex) {
            print("Error executing command: " + ex.getMessage());
        }
    }


}
