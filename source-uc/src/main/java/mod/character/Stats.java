package mod.character;

import net.bytebuddy.asm.Advice;
import zombie.characters.CharacterStat;

public class Stats {
	
	@Advice.OnMethodExit
	public static float get(@Advice.Argument(0) CharacterStat stat,
			@Advice.Return(readOnly = false) float ogReturn) {
		if(stat.equals(CharacterStat.PANIC)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.STRESS)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.UNHAPPINESS)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.DISCOMFORT)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.FATIGUE)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.ENDURANCE)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.STRESS)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.SICKNESS)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.INTOXICATION)) {
			return 0.0f;
		}
		if(stat.equals(CharacterStat.PAIN)) {
			return 0.0f;
		}
		return ogReturn;
	}
}
