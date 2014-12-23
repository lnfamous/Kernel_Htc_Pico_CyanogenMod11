android_kernel_htc_pico
=======================


this branch 'knock_code' is specific and contains stuff for porting Knock Code
or similar functionality for the HTC Pico.

The aim is to keep the functionality as similar to the Knock Code found in LG G3
as possible.

worklist:
* reads patterns, writes patterns, check
* actually recognizes patterns, check
* recognizes patterns on any part of the screen, check

todo list:
* make this work better; doesn't regocnize 3xxx, 4xxx
* more 'efficient'. if possible (for loop)
* support 3 -> 8 taps (the actual two taps: no! two taps is **not** enough)


this branch 'knock_code' will be merged in android-4.4 or android-5.0 when it is
ready, and everything's been cleared off todo list.


knock\_code\_possible_combos (bold: done, rest: not done)

\================================
||121[1-4]||**131[1-4]**||141[1-4]||
||122[1-4]||**132[1-4]**||**142[1-4]**||
||123[1-4]||**133[1-4]**||**143[1-4]**||
||124[1-4]||**134[1-4]**||144[1-4]||
\================================
||211[1-4]||231[1-4]||241[1-4]||
||212[1-4]||232[1-4]||242[1-4]||
||**213[1-4]**||233[1-4]||**243[1-4]**||
||**214[1-4]**||234[1-4]||**244[1-4]**||
\================================
not checked:
\================================
||311[1-4]||321[1-4]||341[1-4]||
||312[1-4]||322[1-4]||342[1-4]||
||313[1-4]||323[1-4]||343[1-4]||
||314[1-4]||324[1-4]||344[1-4]||
\================================
||411[1-4]||421[1-4]||431[1-4]||
||412[1-4]||422[1-4]||432[1-4]||
||413[1-4]||423[1-4]||433[1-4]||
||414[1-4]||424[1-4]||434[1-4]||
\================================
