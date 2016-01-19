# DC_Text_Watch_Deluxe
DC's Text Watch Deluxe for Pebble Time Round smartwatch

<IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413102/3ad8d490-be41-11e5-9bcd-ba3b6f997fb2.gif" ALT="Time-Slide.gif" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413715/c2bb740e-be45-11e5-9028-990f1f120c53.gif" ALT="Time_Dial.gif" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413109/47db7bc0-be41-11e5-926f-4de648e1592c.gif" ALT="Seasons.gif" WIDTH=180 HEIGHT=180>

<IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12381789/d811c75a-bd45-11e5-9ceb-5e4b993f6339.png" ALT="Screenshot_1_0631_Summer.png" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413112/4d01b632-be41-11e5-97d2-d9e387b711ea.png" ALT="surf.png" WIDTH=180 HEIGHT=180> --- <IMG SRC="https://cloud.githubusercontent.com/assets/16750280/12413115/4fe2e3ee-be41-11e5-9cf2-3ae9768d1a57.png" ALT="neapolitan.png" WIDTH=180 HEIGHT=180>

This is a variant of the "Sliding Time" text watch, with some significant changes:
*  Single digit times can be prefixed with O' (same line) or with "Oh" (on an extra line).
*  Rather than "Twelve O'clock", it will say "Twelve Noon" or "Twelve Midnight".
*  The time display will always be centered vertically, whether it is 2 lines or 3 lines.  The transition is animated.
*  The day of the week is fully spelled out and shown in a large font, with proper capitalization.
*  The date has the month abbreviated for space concerns.  The proper ordinal suffix is used (e.g. "1st" rather than "1"). 
*  A subtle analog clock feature is incorporated.  A semicircle transverses the perimeter of the screen once a day, with midnight on the bottom and noon on top.
*  Two subtle dots along the perimeter identify the sunrise and sunset times.  These are computed on the watch, not looked up from a webservice.
*  To compute sunrise & sunset, it must determine location from phone.  Sunrise & sunset indicator dots will not appear on screen until this has been successful.  Attempt will be made every minute until first success, with updates requested every hour afterwards.
*  When successfully getting location from phone, time zone is recorded, as well as if daylight savings time is in affect or not.  If either of these changes, but location has not been updated (e.g. cellular connection updated time, but GPS is switched off), sunrise & sunset should correct to approximate local time.
*  Colors of each element are configurable.

See projects I've starred for various bits of code I've copied from.

By the way, I don't really know what I'm doing; I'm a mechanical systems designer by trade, not a coder.

<IMG SRC="http://imgs.xkcd.com/comics/code_quality.png" ALT="XKCD #1513 on Code Quality" WIDTH=740 HEIGHT=258>