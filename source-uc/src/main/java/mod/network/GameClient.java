package mod.network;

import net.bytebuddy.asm.Advice;
import zombie.characters.IsoPlayer;

public class GameClient {
	
	@Advice.OnMethodExit
	public static void run(
			@Advice.This IsoPlayer player,
			@Advice.Return(readOnly = false) boolean ogReturn) {		
		if(!player.equals(IsoPlayer.getInstance())) {
			ogReturn = true;
		}else {
			System.out.println("im the original player");
		}
//		if(zombie.network.GameClient.instance != null) {
//			for(IsoPlayer player : zombie.network.GameClient.instance.getPlayers()) {
//				if(player != null) {
//					player.setHighlighted(true);				
//					player.isHighlighted();
//				}
//			}			
//		}
	}
}
