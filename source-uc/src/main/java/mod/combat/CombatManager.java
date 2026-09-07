package mod.combat;

import java.util.Random;
import java.util.concurrent.ThreadLocalRandom;

import mod.debug.Debug;
import net.bytebuddy.asm.Advice;
import zombie.characters.IsoGameCharacter;
import zombie.characters.IsoPlayer;
import zombie.characters.IsoZombie;
import zombie.core.physics.RagdollBodyPart;

public class CombatManager {

	public static class applyRangeHitLocationDamage{
		@Advice.OnMethodExit
	    public static void run(
	        @Advice.Argument(0) IsoGameCharacter target,
	        @Advice.Return(readOnly = false) float calculatedDamage
	    ) {
			if(!target.equals(IsoPlayer.getInstance())) {
				Debug.Log("Damage mod is : " + calculatedDamage);
//				calculatedDamage *= 2.0f;
				Debug.Log("Damage mod is : " + calculatedDamage);
			}else {
				if(calculatedDamage > 0) {
					calculatedDamage /= 2;
				}
			}
		}
	}


	public static class processTargetedHit{
		@Advice.OnMethodEnter
	    public static void run(
	        @Advice.Argument(2) IsoGameCharacter target, @Advice.Argument(value = 3, readOnly = false) RagdollBodyPart targetBodyPart
	    ) {
			if(targetBodyPart != RagdollBodyPart.BODYPART_HEAD && !IsoPlayer.getInstance().equals(target)) {
				if(target instanceof IsoPlayer) {
					Debug.Log("Random hit location active");
					int chance = ThreadLocalRandom.current().nextInt(0, 3);
					if (chance == 0) {
					    Debug.Log("Always HeadShot active");
						targetBodyPart = RagdollBodyPart.BODYPART_HEAD;
					}
					if (chance == 1) {
					    Debug.Log("Always Upper Leg active");
					    int chanceLeg = ThreadLocalRandom.current().nextInt(0, 2);
					    if(chanceLeg == 0) {
					    	targetBodyPart = RagdollBodyPart.BODYPART_LEFT_UPPER_LEG;
					    }else {
					    	targetBodyPart = RagdollBodyPart.BODYPART_RIGHT_UPPER_LEG;
					    }
					}
				}
				if(target instanceof IsoZombie) {
					Debug.Log("Always HeadShot active");
					targetBodyPart = RagdollBodyPart.BODYPART_HEAD;
				}

			}

	    }
	}
}
