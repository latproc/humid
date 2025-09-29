
# Linking properties

- objects have default linkage for properties such as visibility
- individual properties can be marked for linkage with a property exported from clockwork


# GENERATE (future)

Example usage:

    GENERATE BUTTON {
      NAME "button_${i}";
      INDEX i RANGE 1 ..  3;
      PROPERTIES (
    	x: i*80;
        y: 40,
        width: 60,
        height: 60
      )
    }
    
    Screen_Untitled_003 STRUCTURE EXTENDS SCREEN {
    }
