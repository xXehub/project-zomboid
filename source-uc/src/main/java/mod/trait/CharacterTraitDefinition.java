package mod.trait;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.Argument;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.characters.IsoPlayer;
import zombie.characters.traits.ObservationFactory;
import zombie.scripting.objects.CharacterTrait;

public class CharacterTraitDefinition {

	public static Map<CharacterTrait, zombie.characters.traits.CharacterTraitDefinition> characterTraitDefinitions =
			new LinkedHashMap<CharacterTrait, zombie.characters.traits.CharacterTraitDefinition>();

	public static Map<CharacterTrait, Boolean> traits =  new LinkedHashMap();
	public static List<CharacterTrait> knownTraits = new ArrayList<CharacterTrait>();

	public static boolean finish = false;

	public static Map<String, Integer> traitsAllowed = new LinkedHashMap();

	public static void getCharacterTraitsByReflection() {
		try {
			if(!finish) {
				init();
				traits.clear();
	            knownTraits.clear();

	            Class<?> clazz = Class.forName("zombie.characters.traits.CharacterTraitDefinition");

	            Field var = clazz.getDeclaredField("characterTraitDefinitions");

	            var.setAccessible(true);

	            characterTraitDefinitions = (LinkedHashMap<CharacterTrait, zombie.characters.traits.CharacterTraitDefinition>) var.get(null);

	            System.out.println("Trait map loaded via reflection: " + characterTraitDefinitions.size() + " entries.");

	            characterTraitDefinitions.forEach((key, value) -> {
	            	System.out.println("Trait: " + key.getName() + " Cost: " + value.getCost());
	            });

	            System.out.println("Current traits: " + IsoPlayer.getInstance().getCharacterTraits().getTraits().size() + " entries.");
	            System.out.println("Known traits: " + IsoPlayer.getInstance().getCharacterTraits().getKnownTraits().size() + " entries.");

	            traits = IsoPlayer.getInstance().getCharacterTraits().getTraits();
	            knownTraits = IsoPlayer.getInstance().getCharacterTraits().getKnownTraits();

	            characterTraitDefinitions.forEach((tt, def) -> {
	            	for(var entry : IsoPlayer.getInstance().getCharacterTraits().getTraits().entrySet()) {
	            		if(entry.getKey().equals(tt)) {
	        				if(def.getCost() < 1 && !traitsAllowed.containsKey(entry.getKey().getName()) && entry.getValue()) {
	        					traits.put(tt, false);
	        					knownTraits.remove(tt);
	        				}
	        				if((def.getCost() >= 1 || traitsAllowed.containsKey(entry.getKey().getName())) && !entry.getValue()) {
	        					traits.put(tt, true);
	        					knownTraits.add(tt);
	        				}
	        				return;
	        			}
	            	}

	            });

	            finish = true;
			}
        } catch (Exception e) {
            System.out.println("Reflection failed: " + e.getMessage());
        }
	}

	public static class Get{
		@Advice.OnMethodExit
		public static void run(@Advice.Argument(0) CharacterTrait characterTrait, @Advice.Return(readOnly = false) boolean ogValue) {
			if(finish) {
				ogValue = traits.get(characterTrait);
			}
		}
	}

	public static class GetKnownTraits{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) List<CharacterTrait> ogValue) {
			if(finish) {
				ogValue = new ArrayList(knownTraits);
			}
		}
	}



	@RuntimeType
	public static List<CharacterTrait> getKnownTraits() {
		return new ArrayList(knownTraits);
	}

	public static void init() {
		traitsAllowed.clear();

		traitsAllowed.put("nutritionist2", 0);
		traitsAllowed.put("brave", 4);
		traitsAllowed.put("fasthealer", 6);
		traitsAllowed.put("obese", 0);
		traitsAllowed.put("artisan", 2);
		traitsAllowed.put("strong", 10);
		traitsAllowed.put("gardener", 2);
		traitsAllowed.put("thickskinned", 8);
		traitsAllowed.put("gymnast", 5);
		traitsAllowed.put("inventive", 2);
		traitsAllowed.put("mechanics2", 0);
		traitsAllowed.put("crafty", 3);
		traitsAllowed.put("fastlearner", 6);
		traitsAllowed.put("outdoorsman", 2);
		traitsAllowed.put("irongut", 3);
		traitsAllowed.put("hiker", 6);
		traitsAllowed.put("nutritionist", 4);
		traitsAllowed.put("cook2", 0);
		traitsAllowed.put("organized", 4);
		traitsAllowed.put("underweight", 0);
		traitsAllowed.put("fit", 6);
		traitsAllowed.put("marksman", 0);
		traitsAllowed.put("lowthirst", 2);
		traitsAllowed.put("dextrous", 2);
		traitsAllowed.put("graceful", 4);
		traitsAllowed.put("formerscout", 6);
		traitsAllowed.put("axeman", 0);
		traitsAllowed.put("mason", 2);
		traitsAllowed.put("mechanics", 3);
		traitsAllowed.put("wildernessknowledge", 8);
		traitsAllowed.put("whittler", 2);
		traitsAllowed.put("desensitized", 0);
		traitsAllowed.put("burglar", 0);
		traitsAllowed.put("needslesssleep", 2);
		traitsAllowed.put("inconspicuous", 4);
		traitsAllowed.put("brawler", 6);
		traitsAllowed.put("lighteater", 2);
		traitsAllowed.put("fastreader", 2);
		traitsAllowed.put("jogger", 4);
		traitsAllowed.put("eagleeyed", 4);
		traitsAllowed.put("adrenalinejunkie", 4);
		traitsAllowed.put("athletic", 10);
		traitsAllowed.put("keenhearing", 6);
		traitsAllowed.put("nightowl", 0);
		traitsAllowed.put("tailor", 4);
		traitsAllowed.put("blacksmith2", 0);
		traitsAllowed.put("handy", 8);
		traitsAllowed.put("blacksmith", 6);
		traitsAllowed.put("herbalist", 4);
		traitsAllowed.put("stout", 6);
		traitsAllowed.put("hunter", 8);
		traitsAllowed.put("baseballplayer", 4);
		traitsAllowed.put("nightvision", 2);
		traitsAllowed.put("speeddemon", 1);
		traitsAllowed.put("resilient", 4);
		traitsAllowed.put("fishing", 4);
		traitsAllowed.put("cook", 3);
		traitsAllowed.put("firstaid", 4);
	}
}
