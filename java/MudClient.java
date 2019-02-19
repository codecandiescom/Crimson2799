/*
 * Java Mud/Telnet Client
 */

import java.awt.*;
import java.applet.*;
import java.util.*;
import java.lang.*;
import java.net.*;
import java.io.*;

public class MudClient extends Applet {
  MudClientPanel clientPanel;

  public void init() {
    setLayout(new BorderLayout());
    MudClientPanel clientPanel = new MudClientPanel(this);
    add("Center", clientPanel);
    add("South",new MudClientControls(clientPanel));
  }

  public boolean handleEvent(Event e) {
    switch (e.id) {
      case Event.WINDOW_DESTROY:
        System.exit(0);
        return true;
      default:
        return false;
    }
  }

  public static void main(String args[]) {
    Frame f = new Frame("MudClient");
    MudClient mudClient = new MudClient();
    mudClient.init();
    mudClient.start();

    f.add("Center", mudClient);
    f.resize(590, 418);
    f.show();
  }

  public void start() {
    //super.start();
    if (clientPanel != null)
      clientPanel.start();
  }

  public void stop() {
    if (clientPanel != null)
      clientPanel.stop();
    //super.stop();
  }

/*
    public boolean mouseDown(Event e, int x, int y) {
      if (frozen) {
        frozen = false;
        start();
      } else {
        frozen = true;
        stop();
      }
      return true;
    }
*/

}



class MudClientPanel extends Panel implements Runnable {
  Image         blitBuffer;
  int           blitHeight;
  int           blitWidth;
  char          intensity;
  char          fg;
  char          bg;
  char          col;
  char          row;
  int           maxRow;
  int           maxCol;
  int           w;
  int           a;
  int           d;
  int           h;  /* a + d */
  Font          font;
  Applet        observer;
  Socket        socket;
  Thread        monitor; /* monitor the socket */
  boolean       frozen;  /* background thread is frozen */
  String        s;
  Color         dkGray;
  Color         red;
  Color         green;
  Color         yellow;
  Color         blue;
  Color         purple;
  Color         cyan;
  Color         white;
  Color         black;
  Color         dkRed;
  Color         dkGreen;
  Color         brown;
  Color         dkBlue;
  Color         dkPurple;
  Color         dkCyan;
  Color         silver;
  Rectangle     r;

  public MudClientPanel(Applet observer) {
    this.observer = observer;
    intensity = '0';
    fg = '7';
    bg = '0';
    dkGray   = new Color(128,128,128); /* dk gray */
    red      = new Color(255,0,0); /* red */
    green    = new Color(0,255,0); /* green */
    yellow   = new Color(255,255,0); /* yellow */
    blue     = new Color(128,128,255); /* blue */
    purple   = new Color(255,0,255); /* purple */
    cyan     = new Color(0,255,255); /* cyan */
    white    = new Color(255,255,255); /* white */
    black    = new Color(0,0,0); /* black */
    dkRed    = new Color(128,0,0); /* dk red */
    dkGreen  = new Color(0,128,0); /* dk green */
    brown    = new Color(128,128,0); /* brown */
    dkBlue   = new Color(0,0,128); /* dk blue */
    dkPurple = new Color(128,0,128); /* dk purple */
    dkCyan   = new Color(0,128,128); /* dk cyan */
    silver   = new Color(192,192,192); /* silver */
    maxRow = 25;
    maxCol = 80;
    col = 0;
    row = 0;
    frozen = false;
    setBackground(Color.black);
    try {
      socket = new Socket("daydream.uvic.ca", 5000);
      socket.setTcpNoDelay(true);
    } catch (IOException ioe) {
      System.out.println("new Socket(): IO Exception");
    }
    start(); /* start the background socket monitor thread */
  }

  /* (re)init the offscreen blitbuffer */
  public void initBlitImage() {
    char  c[] = new char[1];
  
    /* draw the current lines */
    r = bounds();
    Graphics g = getGraphics();
    font = new Font("Courier", Font.PLAIN, 12);
    g.setFont(font);
    FontMetrics fm = g.getFontMetrics();
    w = fm.charWidth('M');
    a = fm.getMaxAscent();
    d = fm.getMaxDescent();
    h = fm.getHeight();  /* a + d */

    blitWidth = w*this.maxCol;
    blitHeight = h*this.maxRow;
    blitBuffer = createImage(blitWidth, blitHeight);
    g = blitBuffer.getGraphics();
    g.setFont(font);
    g.setPaintMode();
    for (int j=0; j < this.maxRow; j++) {
      for (int i=0; i < this.maxCol; i++) {
        g.setColor(getBGColor('0'));
        g.fillRect(w*(i), h*(j), w, h);
        g.setColor(getFGColor('0', '7'));
        c[0] = ' ';
        g.drawChars(c, 0, 1, w*(i), h*(j)+a);
      }
    }
  }

  public Color getFGColor(char intensity, char fg) { 
    if (intensity=='1') {
      if (fg=='0') {
        return dkGray;
      } else if (fg=='1') {
        return red;
      } else if (fg=='2') {
        return green;
      } else if (fg=='3') {
        return yellow;
      } else if (fg=='4') {
        return blue;
      } else if (fg=='5') {
        return purple;
      } else if (fg=='6') {
        return cyan;
      } else if (fg=='7') {
        return white;
      }
    } else {
      if (fg=='0') {
        return black;
      } else if (fg=='1') {
        return dkRed;
      } else if (fg=='2') {
        return dkGreen;
      } else if (fg=='3') {
        return brown;
      } else if (fg=='4') {
        return dkBlue;
      } else if (fg=='5') {
        return dkPurple;
      } else if (fg=='6') {
        return dkCyan;
      } else if (fg=='7') {
        return silver;
      }
    }
    return silver; /* by default make it silver */
  }

  public Color getBGColor(char bg) { 
    return getFGColor('0', bg); /* Black by default */
  }

  public void drawChar(char charValue, int i, int j) {
    char  c[] = new char[1];
    Graphics g = blitBuffer.getGraphics();
    g.setFont(font); /* should be set from init */
    g.setPaintMode(); /* should be set from init */

    /* we should 'cache' color changes here???? */
    g.setColor(getBGColor(bg));
    g.fillRect(w*(i), h*(j), w, h);
    g.setColor(getFGColor(intensity, fg));
    c[0] = charValue;
    g.drawChars(c, 0, 1, w*(i), h*(j)+a);
  }

  public void scrollUp() {
    /* Update data */
    this.row-=1;
    /* scroll up a line */
    Graphics g = blitBuffer.getGraphics();
    g.copyArea(0, h, this.blitWidth, (this.maxRow-1)*h, 0, -h);

    /* Erase the next line with current colors */
    g.setColor(getBGColor(bg));
    g.fillRect(0, h*(maxRow-1), w*80, h);

    /* this is too slow of course */
    /*
    int j = maxRow-1;
    for (int i=0;i<this.maxCol;i++) {
      drawChar(' ',i,j);
    }
    */
  }

  /* Comments on ANSI:
   * A carriage return places the cursor on the next line and fills
   * it with spaces with the current fg/bk color
   * \033[k erases from current pos to end of the current line
   * \033[<intensity>;3<fg>;4<bk>m sets colors
   * any of the 3 can be omitted, if omitted color set to default, SO:
   * \033[m sets the color back to \033[0;37;40m
   * (which is silver on black)
   */
  public int appendText(String buf) {
    int        bRead = 0;

    while (bRead < buf.length()) {
      /* go down one line, erase it with current colors */
      if (buf.charAt(bRead)=='\n') {
        this.col=0;
        this.row++;
        /* Only one screen of data - turf overflow */
        if (this.row>=this.maxRow) {
          scrollUp();
        }
        bRead++;
      
      } else if (buf.charAt(bRead)=='\r') {
        /* skip over \r's, (we treat \n's as \r\n) */
        bRead++;
      
      } else if (buf.charAt(bRead)=='\033') {
        /* Translate ANSI */
        if((bRead+1)>=buf.length())
          return bRead;

        if (buf.charAt(bRead+1)!='[') {
          bRead++;
          continue;
        }
 
        if((bRead+2)>=buf.length())
          return bRead;

        /* Find the command */
        int ansiOffset = bRead;
        int ansiLen = 2;

        while (Character.isDigit(buf.charAt(ansiOffset+ansiLen))
              ||buf.charAt(ansiOffset+ansiLen)==';') {

          ansiLen++;

          /* command isnt complete */
          if (ansiOffset+ansiLen>=buf.length())
            return bRead;
        }

        if (buf.charAt(ansiOffset+ansiLen)=='k'
         || buf.charAt(ansiOffset+ansiLen)=='K') { /* erase command */
          for (int i = col; i<maxCol; i++) {
            drawChar(' ', i,this.row);
          }

        } else if (buf.charAt(ansiOffset+ansiLen)=='m') { /* character formatting command */
          this.intensity = '0'; /* Silver */
          this.fg = '7'; /* Silver */
          this.bg = '0'; /* Black */
          /*
           * This is how easy this is in C, but nothing is this easy in Java
           * cmdNum = sscanf(&buf[bRead], "\033[%s;m%s;m%s;m", cmd[1], cmd[2], cmd[3]);
          */
          for (int i = ansiOffset+2; i<=ansiLen+ansiOffset; ) {
            if (buf.charAt(i)=='0') {
              this.intensity = '0';
            } else if (buf.charAt(i) == '1') {
              this.intensity = '1';
            } else if (buf.charAt(i) == '3') {
              if ( Character.isDigit(buf.charAt(i+1)) ) {
                this.fg = buf.charAt(i+1);
                i++;
              }
            } else if (buf.charAt(i) == '4') {
              if ( Character.isDigit(buf.charAt(i+1)) ) {
                this.bg = buf.charAt(i+1);
                i++;
              }
            }
            i++;
          }
        
        }

        /* mark ansi commands as processed regardless */
        bRead+=ansiLen+1;
    
      /* Guard against control chars */
      } else if (buf.charAt(bRead)>=26){
      /* write character out with current color */
        if (this.col>=this.maxCol) {
          this.col=0;
          this.row+=1;
        }
        if (this.row>=this.maxRow) {
          scrollUp();
        }
        drawChar(buf.charAt(bRead), this.col,this.row);
        bRead++;
        this.col++;
      
      } else {
        /* skip unprintable character */
        bRead++;
      }
    }
    return bRead;
  }

  public boolean handleEvent(Event e) {
    switch (e.id) {
      case Event.WINDOW_DESTROY:
        System.exit(0);
        return true;
      default:
        return false;
    }
  }

  public void start() {
    if (frozen) { 
      //Do nothing.  The user has requested that we 
      //stop changing the image.
    } else {
      //Start animating!
      if (monitor == null) {
        monitor = new Thread(this, "SocketMonitor");
      }
      monitor.start();
    }
  }

  public void stop() {
    //Stop the animating thread.
    monitor.setPriority(Thread.MAX_PRIORITY);
    monitor = null;
  }

  public void run() {
    // UI takes precedence
    Thread.currentThread().setPriority(Thread.MIN_PRIORITY);

    try {
      // Delay start until we can draw
      while (blitBuffer == null)
        Thread.sleep(1);
      while (Thread.currentThread() == monitor) {

        // Anything new
        InputStream is = socket.getInputStream();
        if (is.available()>0) {
          // Read the socket
          if (s==null) s=new String(""); // ensure string is here
          int isAvail = is.available();
          if (isAvail > 128) isAvail = 128; /* update every once in awhile */
          while (isAvail>0 && Thread.currentThread() == monitor) {
            int i = is.read();
            if (i==-1) break; // end of data
            char c = (char) i;
            s = s + String.valueOf(c);
            /*
             * Dont use s.concat(String) because it doesnt fucking work
             */
            if (c=='\n') { //keep the string down to managable size
              // np = NumberProcessed
              int np = appendText(s);
              if (np>0) {
                s = s.substring(np);
                // force redraw screen once a full line has been read in
                Graphics g = getGraphics();
                //should set cliprect here
                //g.clipRect(5, 3, this.blitWidth, this.blitHeight);
                repaint(); 
              }
            }

            isAvail--;
          }

          // np = NumberProcessed
          int np = appendText(s);
          if (np>0) {
            s = s.substring(np);
            // force redraw screen once a full line has been read in
            Graphics g = getGraphics();
            //should set cliprect here
            //g.clipRect(5, 3, this.blitWidth, this.blitHeight);
            repaint();
          }
        }

        Thread.sleep(10); // sleep for a millisecond or two
      }
    } catch (IOException ioe) {
      System.out.println("Thread.InputStream: IOException");
    } catch (InterruptedException ie) {
      System.out.println("Thread.Sleep(): Interrupted Exception");
    }
  }

  public void paint(Graphics g) {
    update(g);
  }

  public void update(Graphics g) {
    if (blitBuffer == null)
      initBlitImage();

    /* Draw the borders around the terminal area */
    //g.setColor(dkBlue);
    //g.fillRect(0, 0, r.width, 3); /* top */
    //g.fillRect(0, 3, 5, r.height-(3)); /* left */
    //g.fillRect(blitWidth+5, 3, r.width-(blitWidth+5), r.height-(3)); /* right */
    //g.fillRect(5, (blitHeight+3), blitWidth, r.height-(blitHeight+3)); /* bottom */

    /* copy blitBuffer */
    g.drawImage(blitBuffer, 5, 3, this.observer);
  }
}


class MudClientControls extends Panel {
  MudClientPanel target;

  public MudClientControls(MudClientPanel target) {
    this.target = target;
    setLayout(new FlowLayout());
    setBackground(Color.lightGray);
    target.setForeground(Color.black);

    TextField tf = new TextField("", 80);
    add(tf);   

/*
    CheckboxGroup group = new CheckboxGroup();
    Checkbox b;
    add(b = new Checkbox(null, group, false));
    b.setBackground(Color.red);
    add(b = new Checkbox(null, group, false));
    b.setBackground(Color.green);
    add(b = new Checkbox(null, group, false));
    b.setBackground(Color.blue);
    add(b = new Checkbox(null, group, false));
    b.setBackground(Color.pink);
    add(b = new Checkbox(null, group, false));
    b.setBackground(Color.orange);
    add(b = new Checkbox(null, group, true));
    b.setBackground(Color.black);
    target.setForeground(b.getForeground());
/*
/*
    Choice shapes = new Choice();
    shapes.addItem("Lines");
    shapes.addItem("Points");
    shapes.setBackground(Color.lightGray);
    add(shapes);
*/
  }

  public void paint(Graphics g) {
    Rectangle r = bounds();

    g.setColor(Color.lightGray);
    g.draw3DRect(0, 0, r.width, r.height, false);
  }

  public boolean action(Event e, Object arg) {
    if (e.target instanceof TextField) {
        /* do something with typing here */
      TextField tf = (TextField)e.target;
      String s = tf.getText();
      tf.setText("");

      // send s over target.socket 
      try {
        OutputStream os = target.socket.getOutputStream();
        for (int i=0; i<s.length(); i++)
          os.write(s.charAt(i));
        os.write('\n');
      } catch (IOException ioe) {
        System.out.println("Write OutputStream: IOException");
      }
   
      /* repaint the target with updated text */
      // Is java re-entrant I wonder?
      target.frozen = true;
      target.stop();
      target.appendText("\n");
      target.repaint();
      target.frozen = false;
      target.start();
 
    } else if (e.target instanceof Choice) {
      /*
      String choice = (String)arg;
      if (choice.equals("Lines")) {
        target.setDrawMode(MudClientPanel.LINES);
      } else if (choice.equals("Points")) {
        target.setDrawMode(MudClientPanel.POINTS);
      }
      */

    }
    return true;
  }
}