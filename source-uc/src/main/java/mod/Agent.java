package mod;

import java.io.File;
import java.lang.instrument.Instrumentation;
import java.net.URISyntaxException;

import mod.character.IsoGameCharacter;
import mod.character.Moodles;
import mod.character.Stats;
import mod.combat.CombatManager;
import mod.debug.Debug;
import mod.esp.Esp;
import mod.inventory.BodyDamage;
import mod.inventory.BodyPart;
import mod.inventory.BuildLogic;
import mod.inventory.Food;
import mod.inventory.HandCraftLogic;
import mod.inventory.InventoryItem;
import mod.inventory.ItemContainer;
import mod.inventory.Literature;
import mod.iso.IsoGridSquare;
import mod.lua.LuaEventManager;
import mod.lua.LuaManager;
import mod.network.GameClient;
import mod.state.IngameState;
import mod.state.MainScreenState;
import mod.trait.CharacterTraitDefinition;
import mod.vehicle.Vehicle;
import mod.weapon.HandWeapon;
import mod.weather.ClimateManager;
import mod.bug.HumanVisualPacket;
import net.bytebuddy.agent.builder.AgentBuilder;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.FixedValue;
import net.bytebuddy.implementation.MethodDelegation;
import net.bytebuddy.implementation.StubMethod;
import net.bytebuddy.matcher.ElementMatchers;

public class Agent {
	
	public static String mainPath;
	
	public static void premain(String agentArgs, Instrumentation inst) throws URISyntaxException {
		File agentJar = new File(Agent.class.getProtectionDomain().getCodeSource().getLocation().toURI());
		
		
		mainPath = agentJar.getParent();		
		
		Debug.active = true;
		System.setProperty("java.awt.headless", "false");
		
        new AgentBuilder.Default()
//        .with(AgentBuilder.Listener.StreamWriting.toSystemOut())
            .type(ElementMatchers.named("zombie.inventory.InventoryItem")) // La clase del juego
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
            	builder
//            	.method(ElementMatchers.named("getWeight"))
//            		.intercept(MethodDelegation.withDefaultConfiguration()
//            				.filter(ElementMatchers.named("getWeight"))
//            				.to(InventoryItem.class))
//        		.method(ElementMatchers.named("getActualWeight"))
//        			.intercept(MethodDelegation.withDefaultConfiguration()
//        				.filter(ElementMatchers.named("getActualWeight"))
//        				.to(InventoryItem.class))
//    			.method(ElementMatchers.named("getContentsWeight"))
//        			.intercept(MethodDelegation.withDefaultConfiguration()
//        				.filter(ElementMatchers.named("getContentsWeight"))
//        				.to(InventoryItem.class))
//    			.method(ElementMatchers.named("getHotbarEquippedWeight"))
//        			.intercept(MethodDelegation.withDefaultConfiguration()
//        				.filter(ElementMatchers.named("getHotbarEquippedWeight"))
//        				.to(InventoryItem.class))
//    			.method(ElementMatchers.named("getEquippedWeight"))
//        			.intercept(MethodDelegation.withDefaultConfiguration()
//        				.filter(ElementMatchers.named("getEquippedWeight"))
//        				.to(InventoryItem.class))
//    			.method(ElementMatchers.named("getUnequippedWeight"))
//        			.intercept(MethodDelegation.withDefaultConfiguration()
//        				.filter(ElementMatchers.named("getUnequippedWeight"))
//        				.to(InventoryItem.class))
    			.method(ElementMatchers.named("getCondition"))
        			.intercept(FixedValue.value(100))
    			.method(ElementMatchers.named("isBroken"))
        			.intercept(FixedValue.value(false))
        			
    			.visit(
        				Advice.to(InventoryItem.Sharpness.class)
        				.on(ElementMatchers.named("getSharpness")))      
    			.visit(
        				Advice.to(InventoryItem.GetCurrentAmmoCount.class)
        				.on(ElementMatchers.named("getCurrentAmmoCount")))      
        	)
//            .type(ElementMatchers.named("zombie.characters.IsoGameCharacter$XP"))
//            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
//        	builder
//        	.visit(
//        			Advice.to(IsoGameCharacter.XP.getMultiplier.class)
//        					.on(ElementMatchers.named("getMultiplier").and(ElementMatchers.takesArguments(1))))
//        	.visit(
//        			Advice.to(IsoGameCharacter.XP.getPerkBoost.class)
//        					.on(ElementMatchers.named("getPerkBoost")))
//    		)
                      
            .type(ElementMatchers.named("zombie.characters.IsoGameCharacter"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("getHitChancesMod"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getHitChancesMod"))
    				.to(IsoGameCharacter.class))
    		.method(ElementMatchers.named("getRecoveryMod"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getRecoveryMod"))
    				.to(IsoGameCharacter.class))
    		.method(ElementMatchers.named("getHittingMod"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getHittingMod"))
    				.to(IsoGameCharacter.class))
//    		.method(ElementMatchers.named("getMaxWeight"))
//    		.intercept(MethodDelegation.withDefaultConfiguration()
//    				.filter(ElementMatchers.named("getMaxWeight"))
//    				.to(IsoGameCharacter.class))
    		.method(ElementMatchers.named("getFatigueMod"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getFatigueMod"))
    				.to(IsoGameCharacter.class))
//    		.method(ElementMatchers.named("getInventoryWeight"))
//    		.intercept(MethodDelegation.withDefaultConfiguration()
//    				.filter(ElementMatchers.named("getInventoryWeight"))
//    				.to(IsoGameCharacter.class))
    		.method(ElementMatchers.named("addStiffness"))
    		.intercept(StubMethod.INSTANCE)    		
//    		.method(ElementMatchers.named("getMaxWeight"))
//    		.intercept(FixedValue.value(80))
//    		.method(ElementMatchers.named("getMaxWeightBase"))
//    		.intercept(FixedValue.value(80))
    		.method(ElementMatchers.named("calculateBaseSpeed"))
    		.intercept(FixedValue.value(2.0f))    		
    		.method(ElementMatchers.named("calculateInjurySpeed"))
    		.intercept(FixedValue.value(0.0f))
    		.method(ElementMatchers.named("calcRunSpeedModByBag"))
    		.intercept(FixedValue.value(10.0f))    		
    		.method(ElementMatchers.named("calculateCombatSpeed"))
    		.intercept(FixedValue.value(1.2f))    		
    		.method(ElementMatchers.named("getFootInjurySpeedModifier"))
    		.intercept(FixedValue.value(0.0f))
    		.method(ElementMatchers.named("canSprint"))    		
    		.intercept(FixedValue.value(true))
    		.method(ElementMatchers.named("isOverEncumbered"))    		
    		.intercept(FixedValue.value(false))    		
//    		.method(ElementMatchers.named("getTimedActionTimeModifier"))    		
//    		.intercept(FixedValue.value(0.2f))    		
        	.visit(
        			Advice.to(IsoGameCharacter.getPerkLevel.class)
        					.on(ElementMatchers.named("getPerkLevel")))        	        
//        	.visit(
//        			Advice.to(IsoGameCharacter.isRecipeKnown.class)
//        					.on(ElementMatchers.named("isRecipeKnown")))
        	.visit(
        			Advice.to(IsoGameCharacter.getFootInjurySpeedModifier.class)
        					.on(ElementMatchers.named("getFootInjurySpeedModifier")))        	
    		)
            .type(ElementMatchers.named("zombie.characters.IsoPlayer"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("isAllowSprint"))
        	.intercept(FixedValue.value(true))
        	.visit(
        			Advice.to(IsoGameCharacter.calculateCritChance.class)
        					.on(ElementMatchers.named("calculateCritChance")))
        	)
            .type(ElementMatchers.named("zombie.CombatManager"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder        	
        	.visit(
        			Advice.to(CombatManager.processTargetedHit.class)
        					.on(ElementMatchers.named("processTargetedHit")))
        	.visit(
        			Advice.to(CombatManager.applyRangeHitLocationDamage.class)
        					.on(ElementMatchers.named("applyRangeHitLocationDamage")))
        	)
            .type(ElementMatchers.named("zombie.inventory.types.HandWeapon"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("getCriticalChance"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getCriticalChance"))
    				.to(HandWeapon.class))
    		.method(ElementMatchers.named("getHitChance"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getHitChance"))
    				.to(HandWeapon.class))
    		.method(ElementMatchers.named("getAimingTime"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getAimingTime"))
    				.to(HandWeapon.class))
    		.visit(
    				Advice.to(HandWeapon.GetDamageMod.class)
    				.on(ElementMatchers.named("getDamageMod")))
    		.visit(
    				Advice.to(HandWeapon.GetRangedMod.class)
    				.on(ElementMatchers.named("getRangeMod")))
//    		.method(ElementMatchers.named("getDamageMod"))
//    		.intercept(FixedValue.value(2.0f))
    		.method(ElementMatchers.named("getWeaponSkill"))
    		.intercept(FixedValue.value(10))    		
//    		.method(ElementMatchers.named("getMaxAngle"))
//    		.intercept(FixedValue.value(0.1f))
//    		.method(ElementMatchers.named("getMinAngle"))
//    		.intercept(FixedValue.value(0.1f))    		
//    		.visit(
//    				Advice.to(HandWeapon.ReloadTime.class)
//    				.on(ElementMatchers.named("getReloadTime")))
    		.visit(
    				Advice.to(HandWeapon.MaxRange.class)
    				.on(ElementMatchers.named("getMaxRange")))
    		.visit(
    				Advice.to(HandWeapon.IsRoundChambered.class)
    				.on(ElementMatchers.named("isRoundChambered")))
    		.visit(
    				Advice.to(HandWeapon.GetRecoilDelay.class)
    				.on(ElementMatchers.named("getRecoilDelay")
    						.and(ElementMatchers.takesArguments(0))))
    		)            
            .type(ElementMatchers.named("zombie.inventory.ItemContainer"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
//        	.visit(
//    				Advice.to(ItemContainer.class)
//    				.on(ElementMatchers.named("haveThisKeyId")))
//        	)
        	.method(ElementMatchers.named("getCapacity"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getCapacity"))
    				.to(ItemContainer.class))
    		.method(ElementMatchers.named("getWeightReduction"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getWeightReduction"))
    				.to(ItemContainer.class))
    		.method(ElementMatchers.named("getMaxWeight"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getMaxWeight"))
    				.to(ItemContainer.class))
    		.method(ElementMatchers.named("getCapacityWeight"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("getCapacityWeight"))
    				.to(ItemContainer.class))
    		)            
            .type(ElementMatchers.named("zombie.inventory.types.Literature"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(Literature.class)
        					.on(ElementMatchers.named("getNumberOfPages")))
        	.method(ElementMatchers.named("getMaxLevelTrained"))
    		.intercept(FixedValue.value(0))
        	)
            .type(ElementMatchers.named("zombie.characters.Stats"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(Stats.class)
        					.on(ElementMatchers.named("get")
        							.and(ElementMatchers.takesArguments(1))))        	
        	) 
            .type(ElementMatchers.named("zombie.gameStates.MainScreenState"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(MainScreenState.class)
        					.on(ElementMatchers.named("enter")))        	
        	)
            .type(ElementMatchers.named("zombie.gameStates.IngameState"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(IngameState.class)
        					.on(ElementMatchers.named("enter")))        	
        	)
            .type(ElementMatchers.named("zombie.characters.BodyDamage.BodyPart"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("getStiffness"))
    		.intercept(FixedValue.value(0.0f))
//    		.method(ElementMatchers.named("HasInjury"))
//    		.intercept(FixedValue.value(false))
    		.method(ElementMatchers.named("getAdditionalPain"))
    		.intercept(FixedValue.value(0.0f))
    		.method(ElementMatchers.named("getPain"))
    		.intercept(FixedValue.value(0.0f))    		
    		.method(ElementMatchers.named("getFractureTime"))
    		.intercept(FixedValue.value(0.0f))
    		.method(ElementMatchers.named("ReduceHealth"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("ReduceHealth"))
    				.to(BodyPart.class))
    		)
            .type(ElementMatchers.named("zombie.characters.BodyDamage.BodyDamage"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("setOverallBodyHealth"))
    		.intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("setOverallBodyHealth"))
    				.to(BodyDamage.class))
    		)    		           
            .type(ElementMatchers.named("zombie.characters.Moodles.Moodles"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(Moodles.class)
        					.on(ElementMatchers.named("getMoodleLevel")))        	
        	)
            .type(ElementMatchers.named("zombie.characters.Moodles.Moodle"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(Moodles.updateMoodleLevel.class)
        					.on(ElementMatchers.named("updateMoodleLevel")))
        	)
            .type(ElementMatchers.named("zombie.inventory.types.Food"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(Food.class)
        					.on(ElementMatchers.named("getName")))        	
        	)
            .type(ElementMatchers.named("zombie.Lua.LuaManager"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.method(ElementMatchers.named("checkPermissions"))
    		.intercept(FixedValue.value(true))        	
        	.visit(
        			Advice.to(LuaManager.FinishCheckSum.class)
        					.on(ElementMatchers.named("finishChecksum")))        	
        	)
            .type(ElementMatchers.named("zombie.Lua.LuaEventManager"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder.visit(
        			Advice.to(LuaEventManager.TriggerEvent.class)
        					.on(ElementMatchers.named("triggerEvent")
        							.and(ElementMatchers.takesArguments(2))))        	
        	)
//            .type(ElementMatchers.named("zombie.inventory.types.DrainableComboItem"))
//            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
//        	builder
//        	.method(ElementMatchers.named("getCurrentUsesFloat"))
//    		.intercept(FixedValue.value(1.0f))
//    		)            
            .type(ElementMatchers.named("zombie.characters.traits.CharacterTraits"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.visit(
        			Advice.to(CharacterTraitDefinition.Get.class)
        					.on(ElementMatchers.named("get")
        							.and(ElementMatchers.takesArguments(1))))
        	.visit(
        			Advice.to(CharacterTraitDefinition.GetKnownTraits.class)
        					.on(ElementMatchers.named("getKnownTraits")))
        	)        	
            .type(ElementMatchers.named("zombie.iso.weather.ClimateManager"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
        	builder
        	.visit(
        			Advice.to(ClimateManager.GetNightStrength.class)
					.on(ElementMatchers.named("getNightStrength")))
            )
            .type(ElementMatchers.named("zombie.vehicles.BaseVehicle"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
            builder                    
            .visit(Advice.to(Vehicle.isHotwiredBroken.class)
					.on(ElementMatchers.named("isHotwiredBroken")))
            .method(ElementMatchers.named("getEngineQuality"))
            .intercept(FixedValue.value(90))
            .method(ElementMatchers.named("canUnlockDoor"))
            .intercept(FixedValue.value(true))           
            .method(ElementMatchers.named("updateEngineStarting"))
            .intercept(MethodDelegation.withDefaultConfiguration()
    				.filter(ElementMatchers.named("updateEngineStarting"))
    				.to(Vehicle.class))
            .visit(Advice.to(Vehicle.getBatteryCharge.class)
            		.on(ElementMatchers.named("getBatteryCharge")))
            .visit(Advice.to(Vehicle.getEnginePower.class)
            		.on(ElementMatchers.named("getEnginePower")))
            .visit(Advice.to(Vehicle.getEngineLoudness.class)
            		.on(ElementMatchers.named("getEngineLoudness")))
            .visit(Advice.to(Vehicle.addRandomDamageFromCrash.class)
            		.on(ElementMatchers.named("addRandomDamageFromCrash")))
            .visit(Advice.to(Vehicle.getBrakingForce.class)
            		.on(ElementMatchers.named("getBrakingForce")))            
            
//    		.intercept(MethodDelegation.withDefaultConfiguration()
//    				.filter(ElementMatchers.named("tryStartEngine"))
//    				.to(Vehicle.class)))
////    		.visit(Advice.to(Vehicle.class)
////        					.on(ElementMatchers.named("	")))    		
//    		.visit(Advice.to(Vehicle.Script.class)
//					.on(ElementMatchers.named("getScript")))
//    		)
            )
            .type(ElementMatchers.named("zombie.vehicles.VehiclePart"))
            .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
            builder
            .visit(Advice.to(Vehicle.VehiclePart.getContainerContentAmount.class)
            		.on(ElementMatchers.named("getContainerContentAmount")))
            .visit(Advice.to(Vehicle.VehiclePart.getCondition.class)
            		.on(ElementMatchers.named("getCondition")))
            )
//            .type(ElementMatchers.named("zombie.worldMap.WorldMapVisited"))
//            .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
//            builder
//            .method(ElementMatchers.named("isCellVisible"))
//            .intercept(FixedValue.value(true))
//            .method(ElementMatchers.named("isKnown").and(ElementMatchers.takesArguments(int.class, int.class)))            
//            .intercept(FixedValue.value(true))
//            .method(ElementMatchers.named("isKnown").and(ElementMatchers.takesArguments(int.class, int.class, int.class, int.class)))
//            .intercept(FixedValue.value(true))
//            )
//            .type(ElementMatchers.named("zombie.iso.IsoGridSquare"))
//            .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
//        	builder
//        	.visit(
//        			Advice.to(IsoGridSquare.GetLightInfo.class)
//					.on(ElementMatchers.named("getLightInfo")))
//            )
          .type(ElementMatchers.named("zombie.iso.IsoGridSquare"))
          .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
	      	builder
	      	.visit(
	      			Advice.to(IsoGridSquare.isCanSee.class)
						.on(ElementMatchers.named("isCanSee")))
	          )
          .type(ElementMatchers.named("zombie.ui.UIManager"))
          .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
          builder
          .visit(Advice.to(Esp.class)
          		.on(ElementMatchers.named("render")))
          )
          .type(ElementMatchers.named("zombie.scripting.objects.VehicleScript"))
          .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
          builder
          .visit(Advice.to(Vehicle.VehicleScript.getEngineForce.class)
          		.on(ElementMatchers.named("getEngineForce")))
          )
          .type(ElementMatchers.named("zombie.entity.components.crafting.BaseCraftingLogic"))
          .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
	      builder
	      .method(ElementMatchers.named("cachedCanPerformCurrentRecipe"))
	  	  .intercept(FixedValue.value(true))
		  .method(ElementMatchers.named("canPerformCurrentRecipe"))
		  .intercept(FixedValue.value(true))
		  	  
        	)
          .type(ElementMatchers.named("zombie.network.packets.HumanVisualPacket"))
          .transform((builder, typeDescription, classLoader, module, protectionDomain) -> 
	      builder
	      .method(ElementMatchers.named("process"))
          .intercept(MethodDelegation.withDefaultConfiguration()
  				.filter(ElementMatchers.named("process"))
  				.to(HumanVisualPacket.class))
	  	  )
          
//	      .type(ElementMatchers.named("zombie.entity.components.crafting.recipe.HandcraftLogic"))
//	      .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
//	      builder
//	      .visit(Advice.to(HandCraftLogic.performCurrentRecipe.class)
//	      		.on(ElementMatchers.named("performCurrentRecipe")))
//	      )
//	      .type(ElementMatchers.named("zombie.entity.components.build.BuildLogic"))
//	      .transform((builder, typeDescription, classLoader, module, protectionDomain) ->
//	      builder
//	      .visit(Advice.to(BuildLogic.performCurrentRecipe.class)
//	      		.on(ElementMatchers.named("performCurrentRecipe")))
//	      )
            
            .installOn(inst);
            

    }
}
