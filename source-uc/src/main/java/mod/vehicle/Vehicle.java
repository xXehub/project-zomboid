package mod.vehicle;

import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.Argument;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import net.bytebuddy.implementation.bind.annotation.This;
import zombie.SandboxOptions;
import zombie.characters.IsoGameCharacter;
import zombie.characters.IsoPlayer;
import zombie.core.random.Rand;
import zombie.inventory.InventoryItem;
import zombie.inventory.InventoryItemFactory;
import zombie.iso.weather.ClimateManager;
import zombie.scripting.objects.ItemKey.Key;
import zombie.vehicles.BaseVehicle;
import zombie.vehicles.VehiclePart;

public class Vehicle {

	public static boolean active = true;
	public static boolean infinityTank = false;
	public static boolean repairhotwire = false;

	public static float enginePower = 300.0f;

	@RuntimeType
	public static boolean canOpenDoor(@Advice.FieldValue(value="keyId", readOnly = true) int keyId) {
		System.out.println("Vehicle UI toggled");
		if(IsoPlayer.getInstance().getInventory().haveThisKeyId(keyId) == null) {
			InventoryItem item = InventoryItemFactory.CreateItem(Key.CAR_KEY);
			item.setKeyId(keyId);
			IsoPlayer.getInstance().getInventory().addItem(item);
		}
		return true;
	}

	public static void tryStartEngine() {
		if(IsoPlayer.getInstance().getVehicle() != null) {
			IsoPlayer.getInstance().getVehicle().engineDoStarting();
			IsoPlayer.getInstance().getVehicle().engineDoStartingSuccess();
			IsoPlayer.getInstance().getVehicle().engineDoRunning();
		}
	}

	public static class Script{
		@Advice.OnMethodExit
		public static void getScript() {
			System.out.println("Vehicle script loaded");
		}
	}

	public static class isHotwiredBroken{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) boolean ogValue) {
			if(active && ogValue) {
				ogValue = false;
			}
		}
	}

	@RuntimeType
	public static void updateEngineStarting(@This Object obj) {
		BaseVehicle vehicle = (BaseVehicle) obj;
		if(!active) {
			if (vehicle.getBatteryCharge() <= 0.1F) {
				vehicle.engineDoStartingFailedNoPower();
			} else {
				zombie.vehicles.VehiclePart gasTank = vehicle.getPartById("GasTank");
				if (gasTank != null && gasTank.getContainerContentAmount() <= 0.0F) {
					vehicle.engineDoStartingFailed("VehicleRunningOutOfGas");
				} else {
					int weatherAffect = 0;
					float airTemp = ClimateManager.getInstance().getAirTemperatureForSquare(vehicle.getSquare());
					if (vehicle.getEngineQuality() < 65 && airTemp <= 2.0F) {
						weatherAffect = Math.min((2 - (int) airTemp) * 2, 30);
					}

					if (!SandboxOptions.instance.vehicleEasyUse.getValue() && vehicle.getEngineQuality() < 100
							&& Rand.Next(vehicle.getEngineQuality() + 50 - weatherAffect) <= 30) {
						vehicle.engineDoStartingFailed("VehicleEngineFailureDamage");
					} else {
						if (Rand.Next(vehicle.getEngineQuality()) != 0) {
							vehicle.engineDoStartingSuccess();
						} else {
							vehicle.engineDoRetryingStarting();
						}
					}
				}
			}
		}
	}

	public static class getEnginePower{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) int ogValue) {
			ogValue += enginePower;
		}
	}

	public static class getEngineLoudness{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) int ogValue) {
//			ogValue = 0;
		}
	}


	public static class getBatteryCharge{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) float ogValue) {
			if(active) {
				if(ogValue <= 0.6f) {
					ogValue = 1.0f;
				}
			}
		}
	}

	public static class addRandomDamageFromCrash{
		@Advice.OnMethodEnter
		public static void run(@Advice.Argument(0) IsoGameCharacter chr, @Advice.Argument(value = 1, readOnly = false) float damage) {
			if(chr.equals(IsoPlayer.getInstance())) {
				damage = 0;
			}
		}
	}

	public static class getBrakingForce{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) float ogValue) {
			ogValue *= 3.0f;
		}
	}



	public static class VehiclePart{
		public static class getContainerContentAmount{
			@Advice.OnMethodExit
			public static void run(@Advice.Return(readOnly = false) float ogValue) {
				if(active) {
					if(ogValue <= 1.0f) {
						ogValue = 5.0f;
					}
				}
			}
		}
		public static class getCondition{
			@Advice.OnMethodExit
			public static void run(@Advice.Return(readOnly = false) int ogValue) {
				if(active) {
					if(ogValue < 70) {
						ogValue = 70;
					}
				}
			}
		}
	}

	public static class VehicleScript{
		public static class getEngineForce{
			@Advice.OnMethodExit
			public static void run(@Advice.Return(readOnly = false) float ogValue) {
				ogValue += enginePower;
			}
		}
	}


}
