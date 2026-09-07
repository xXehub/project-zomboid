package mod.radar;

import java.util.ArrayList;

import zombie.characters.IsoPlayer;
import zombie.iso.IsoWorld;
import zombie.network.GameClient;

public class Radar {
	public static class EntityPos {
        public float x, y;
        public String name;
        public boolean isZombie;

        public EntityPos(float x, float y, String name, boolean isZombie) {
            this.x = x; this.y = y; this.name = name; this.isZombie = isZombie;
        }
    }

    public static ArrayList<EntityPos> getNearbyEntities(int radius) {
    	ArrayList<EntityPos> entities = new ArrayList<>();
        IsoPlayer localPlayer = IsoPlayer.getInstance();

        if (localPlayer == null) return entities;

        float playerX = localPlayer.getX();
        float playerY = localPlayer.getY();


        if (GameClient.instance == null) return entities;

        var list = GameClient.instance.getPlayers();


        if (list == null) return entities;

        for (int i = 0; i < list.size(); i++) {
            var character = list.get(i);


            if (character == null || character == localPlayer) continue;

            float dist = (float) Math.sqrt(Math.pow(character.getX() - playerX, 2) + Math.pow(character.getY() - playerY, 2));

            if (dist <= radius) {
                entities.add(new EntityPos(
                    character.getX() - playerX,
                    character.getY() - playerY,
                    character.getObjectName() != null ? character.getObjectName() : "Unknown",
                    character.isZombie()
                ));
            }
        }
        return entities;
    }
}
