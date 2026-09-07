package mod.bug;

import net.bytebuddy.implementation.bind.annotation.Argument;
import net.bytebuddy.implementation.bind.annotation.FieldValue;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import net.bytebuddy.implementation.bind.annotation.This;
import zombie.characters.IsoPlayer;
import zombie.core.logger.ExceptionLogger;
import zombie.core.network.ByteBufferWriter;
import zombie.core.raknet.UdpConnection;
import zombie.network.GameClient;
import zombie.network.GameServer;
import zombie.network.PacketTypes.PacketType;
import zombie.network.fields.character.PlayerID;

public class HumanVisualPacket {
	@RuntimeType
	public static void process(@This Object obj, @FieldValue("player") PlayerID player, @Argument(0) UdpConnection connection) {
		zombie.network.packets.HumanVisualPacket othis = (zombie.network.packets.HumanVisualPacket) obj;
		
		if (GameServer.server) {
			for (int n = 0; n < GameServer.udpEngine.connections.size(); ++n) {
				UdpConnection c = (UdpConnection) GameServer.udpEngine.connections.get(n);
				if (c.getConnectedGUID() != connection.getConnectedGUID()) {
					IsoPlayer p2 = GameServer.getAnyPlayerFromConnection(c);
					if (p2 != null) {
						ByteBufferWriter b2 = c.startPacket();
						PacketType.HumanVisual.doPacket(b2);

						try {
							othis.write(b2);
							PacketType.HumanVisual.send(c);
						} catch (RuntimeException var7) {
							c.cancelPacket();
							ExceptionLogger.logException(var7);
						}
					}
				}
			}
		}		
		
		if (GameClient.client) {
			if(player != null) {
				if(player.getPlayer() != null) {
					player.getPlayer().resetModelNextFrame();		
				}
			}			
		}

	}
}
