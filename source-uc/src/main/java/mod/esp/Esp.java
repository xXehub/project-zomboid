package mod.esp;

import zombie.characters.IsoPlayer;
import zombie.characters.IsoZombie;
import zombie.core.Core;
import zombie.core.SpriteRenderer;
import zombie.core.textures.Texture;
import zombie.iso.IsoCamera;
import zombie.iso.IsoUtils;
import zombie.iso.IsoWorld;
import zombie.network.GameClient;

import java.util.ArrayList;

import net.bytebuddy.asm.Advice;

public class Esp {
    public static boolean enabled = true;

    @Advice.OnMethodExit
    public static void renderOverlay() {
        if (!enabled) return;

        IsoPlayer localPlayer = IsoPlayer.getInstance();
        if (localPlayer == null) return;

        ArrayList<IsoZombie> zombies = IsoWorld.instance.currentCell.getZombieList();

        if (zombies != null && !zombies.isEmpty())
        {
        	int playerIndex = localPlayer.getPlayerNum();

            Texture whiteTexture = Texture.getWhite();

            if (whiteTexture == null) return;

            for (int i = 0; i < zombies.size(); i++) {
                IsoZombie zombie = zombies.get(i);

                if (zombie == null || zombie.isDead()) continue;

                float drawX = zombie.getX();
                float drawY = zombie.getY();
                float drawZ = zombie.getZ();

                float zoom = Core.getInstance().getZoom(playerIndex);

                float screenX = IsoUtils.XToScreen(drawX, drawY, drawZ, playerIndex);
                float screenY = IsoUtils.YToScreen(drawX, drawY, drawZ, playerIndex);

                screenX -= IsoCamera.getOffX();
                screenY -= IsoCamera.getOffY();

                screenX /= zoom;
                screenY /= zoom;

                screenY -= 35;

                float boxSize = 6.0f;

                SpriteRenderer.instance.render(
                    whiteTexture,
                    screenX - (boxSize / 2), screenY - (boxSize / 2),
                    boxSize, boxSize,
                    1.0f, 0.0f, 0.0f, 1.0f,
                    null
                );
            }
        }

        ArrayList<IsoPlayer> players = GameClient.instance.getPlayers();

        if (players != null && !players.isEmpty())
        {
        	int playerIndex = localPlayer.getPlayerNum();

            Texture whiteTexture = Texture.getWhite();

            if (whiteTexture == null) return;

            for (int i = 0; i < players.size(); i++) {
                IsoPlayer player = players.get(i);

                if (player == null || player.isDead()) continue;

                float drawX = player.getX();
                float drawY = player.getY();
                float drawZ = player.getZ();

                float zoom = Core.getInstance().getZoom(playerIndex);

                float screenX = IsoUtils.XToScreen(drawX, drawY, drawZ, playerIndex);
                float screenY = IsoUtils.YToScreen(drawX, drawY, drawZ, playerIndex);

                screenX -= IsoCamera.getOffX();
                screenY -= IsoCamera.getOffY();

                screenX /= zoom;
                screenY /= zoom;

                screenY -= 35;

                float boxSize = 6.0f;

                SpriteRenderer.instance.render(
                    whiteTexture,
                    screenX - (boxSize / 2), screenY - (boxSize / 2),
                    boxSize, boxSize,
                    0.0f, 0.0f, 1.0f, 1.0f,
                    null
                );
            }
        }
    }
}
