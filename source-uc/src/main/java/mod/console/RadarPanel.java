package mod.console;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JPanel;
import javax.swing.Timer;

import mod.radar.Radar;

public class RadarPanel extends JPanel { 
    public static int radius = 200; 

    public RadarPanel() {
        setPreferredSize(new Dimension(500, 500));
        Timer timer = new Timer(100, e -> this.repaint());
        timer.start();
    }
    
    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2 = (Graphics2D) g;
        
        g2.setColor(Color.BLACK);
        g2.fillRect(0, 0, getWidth(), getHeight());

        int centerX = getWidth() / 2;
        int centerY = getHeight() / 2;
        
        if (getWidth() == 0) return; 
        
        float scale = (float) getWidth() / (radius * 2);

        int[] distRings = {25, 50, 100, 150, 200};
        
        for (int dist : distRings) {
            if (dist <= radius) {
                g2.setColor(new Color(0, 100, 0));
                
                int ringW = (int)(dist * scale * 2);
                int ringH = ringW / 2;
                
                g2.drawOval(centerX - (ringW / 2), centerY - (ringH / 2), ringW, ringH);
                
                
                g2.setColor(new Color(0, 150, 0));
                g2.drawString(dist + "m", centerX + (ringW / 2) + 2, centerY);
            }
        }
        // ------------------------------------------

        g2.setColor(Color.WHITE);
        g2.fillOval(centerX - 3, centerY - 3, 6, 6);

        try {
            var entities = Radar.getNearbyEntities(radius);
            for (Radar.EntityPos ent : entities) {
                
                float isoX = (ent.x - ent.y);
                float isoY = (ent.x + ent.y) / 2.0f; 

                int drawX = centerX + (int)(isoX * scale);
                int drawY = centerY + (int)(isoY * scale);

                if (ent.isZombie) {
                    g2.setColor(Color.RED);
                } else {
                    g2.setColor(Color.BLUE); 
                } 
                g2.fillOval(drawX - 2, drawY - 2, 4, 4);
            }
        } catch (Exception ex) {
            g2.setColor(Color.RED);
            g2.drawString("Radar Error", 10, 20);
        }
    }
}