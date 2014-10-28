NightShift
==========
DIY electromechanical bicycle rear derailleur + control software project

Idea & motivation for this project is to explore and try to recreate the recent innovations in bicycle
technology by combining the cheapest off-shelf bicycle derailleur (Shimano RD-TX31B) with a affordable 
microcontroller (Arduino Nano). Targeted total cost for final build is around 50 euros, while the 
commercially made systems can be bought starting from ~1100 eur.

Hardware preparation and individual component testing is mostly done in wee hours because family,
hence the name "Night shift".

Short overview
--------------
The basic idea is to create a compact backbone or "shield" to connect the sub-systems and Arduino Nano together.
For the clean result, the backbone/Arduino plus the battery pack are designed to be installed inside 
the bicycle's seat post.

![Parts](/Fritzing/nightshift-hardware-general-diagram.png)

About this repository
---------------------
This repository following structure:

Semi-ordered folders of misc. build pics
  * Build pics/
    * Control buttons/
    * Hall sensor/
    * Nano-Backbone/
    * Rear derailleur related/

Schematics to build subsystems
  * Fritzing/

Software (arduino firmware & control software)
  * Säätösofta/
    * NightShift/
    * NightShift_control/
    
Credits
-------
As always, I'm standing on the shoulders of giants. There are some people & other parties involved and without you
I just couldn't pull this thing off.
* My wife & the terrible three (=my lovely kids)
* Mr J.C. for kindly arranging the things in the big picture & keeping the perspective correct
* My employee (for arranging Hackathon -event & monthly funding of my hobbies via paycheck!)
* The nice guy at local electronics shop
* Internet search engine(s)
