package mod.character;

import mod.debug.Debug;
import mod.lua.LuaUtil;
import mod.trait.CharacterTraitDefinition;
import mod.weapon.HandWeapon;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.SandboxOptions;
import zombie.characters.IsoPlayer;
import zombie.characters.skills.PerkFactory;
import zombie.characters.skills.PerkFactory.Perks;

public class IsoGameCharacter {

	public static int boost = 0;

	public static boolean once = false;

	public static boolean artisan = false;

	public static boolean unlockRecipes = true;

	public static boolean perkActive = true;

	@RuntimeType
	public static int getHitChancesMod() {
		return 5;
	}

	@RuntimeType
	public static float getRecoveryMod() {
		if(!once) {
			CharacterTraitDefinition.getCharacterTraitsByReflection();
			IsoGameCharacter.general();
			once = true;
		}
		return 4f;
	}

	@RuntimeType
	public static float getHittingMod() {
		if(HandWeapon.x2) {
			return 3f;
		}
		return 1.5f;
	}


	@RuntimeType
	public static float getFatigueMod() {
		return 0.1f;
	}

	@RuntimeType
	public static float getMaxWeight() {
		return 1000;
	}

	@RuntimeType
	public static float getInventoryWeight() {
		return 1.0f;
	}

	public static void general() {
		SandboxOptions.instance.muscleStrainFactor.setValue(0.0f);
	}

	public static class getPerkLevel{
		@Advice.OnMethodExit
		public static void run(@Advice.Argument(0) PerkFactory.Perk perk,
				@Advice.Return(readOnly = false) int ogReturn) {
			if(perkActive) {
				if(!LuaUtil.isStackFromLua()) {
					if(perk == Perks.Aiming) {
						ogReturn = 10;
					}
					if(perk == Perks.Combat) {
						ogReturn = 10;
					}
					if(perk == Perks.Firearm) {
						ogReturn = 10;
					}
					if(perk == Perks.Reloading) {
						ogReturn = 10;
					}
					if(perk == Perks.Sneak) {
						ogReturn = 10;
					}
					if(perk == Perks.Lightfoot) {
						ogReturn = 10;
					}
					if(perk == Perks.LongBlade) {
						ogReturn = 10;
					}
					if(perk == Perks.SmallBlade) {
						ogReturn = 10;
					}
					if(perk == Perks.Blunt) {
						ogReturn = 10;
					}
					if(perk == Perks.SmallBlunt) {
						ogReturn = 10;
					}
					if(perk == Perks.Melee) {
						ogReturn = 10;
					}
					if(perk == Perks.Axe) {
						ogReturn = 10;
					}
					if(perk == Perks.Spear) {
						ogReturn = 10;
					}
					if(perk == Perks.PhysicalCategory) {
						ogReturn = 10;
					}
					if(perk == Perks.Strength) {
						ogReturn = 10;
					}
					if(perk == Perks.Agility) {
						ogReturn = 10;
					}
					if(perk == Perks.Sprinting) {
						ogReturn = 10;
					}
					if(perk == Perks.Fitness) {
						ogReturn = 10;
					}
					if(perk == Perks.Nimble) {
						ogReturn = 10;
					}

					if(artisan) {
						if(perk == Perks.Electricity) {
							ogReturn = 10;
						}
						if(perk == Perks.Mechanics) {
							ogReturn = 10;
						}
						if(perk == Perks.Crafting) {
							ogReturn = 10;
						}
						if(perk == Perks.Tailoring) {
							ogReturn = 10;
						}
						if(perk == Perks.Maintenance) {
							ogReturn = 10;
						}
						if(perk == Perks.Blacksmith) {
							ogReturn = 10;
						}
						if(perk == Perks.Doctor) {
							ogReturn = 10;
						}
						if(perk == Perks.MetalWelding) {
							ogReturn = 10;
						}
						if(perk == Perks.Trapping) {
							ogReturn = 10;
						}
						if(perk == Perks.Woodwork) {
							ogReturn = 10;
						}
						if(perk == Perks.Carving) {
							ogReturn = 10;
						}
						if(perk == Perks.Butchering) {
							ogReturn = 10;
						}
						if(perk == Perks.Cooking) {
							ogReturn = 10;
						}
						if(perk == Perks.Farming) {
							ogReturn = 10;
						}
						if(perk == Perks.Fishing) {
							ogReturn = 10;
						}
						if(perk == Perks.Survivalist) {
							ogReturn = 10;
						}
					}

				}

			}

		}
	}

	public static class getFootInjurySpeedModifier{
		@Advice.OnMethodExit
		public static void run(@Advice.FieldValue(value="walkSpeedModifier", readOnly = false) float walkSpeedModifier,
				@Advice.FieldValue(value="runSpeedModifier", readOnly = false) float runSpeedModifier,
				@Advice.FieldValue(value="maxWeight", readOnly = false) int maxWeight,
				@Advice.FieldValue(value="maxWeightBase", readOnly = false) int maxWeightBase) {
			walkSpeedModifier = 2.0f;
			runSpeedModifier = 2.0f;
			maxWeight = 80;
			maxWeightBase = 80;
		}
	}

	public static class isRecipeKnown{
		@Advice.OnMethodExit
	    public static void run(
	        @Advice.Return(readOnly = false) boolean isKnown
	    ) {
	        if (unlockRecipes) {
	            isKnown = true;
	        }
	    }
	}

	public static class calculateCritChance{
		@Advice.OnMethodExit
	    public static void run(@Advice.Argument(0) zombie.characters.IsoGameCharacter target,
	        @Advice.Return(readOnly = false) int chance
	    ) {
	        if (IsoPlayer.getInstance().equals(target)) {
	        	Debug.Log("Zero chance to me");
	            chance = 0;
	        }else {
	        	chance = 100;
	        }
	    }
	}

	public static boolean nightVision = false;

	public static void nightVisionToggle() {
		nightVision = !nightVision;
		IsoPlayer.getInstance().setWearingNightVisionGoggles(nightVision);
	}

}
