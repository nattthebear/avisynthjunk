LoadCPlugin ("jmdsource.dll")

/*
any avisynth valid function, user defined from file or include, or any in context plugin, can be used as a resizer for JMDSource
it will be fed three non-named parameters: (clip, int, int)  the first is input clip, BGR32, at its native original resolution.
the second and third are width and height of user's specified destination.

two notes about this example:
one: floats are ok because of default type coalescing
two: PointResize 720x400->640x400 with default parameters matches the "9 to 8" option of the JMD tools

*/



function NineToEight (clip c, float destw, float desth)
{
  return PointResize (c, destw, desth)

}

function SpecialResize (clip c, int w, int h)
{
  PointResize (c, 640, 400)
  LanczosResize (w, h)

}
LoadCPlugin ("timecodefps.dll")

JMDSource ("..\jpcrr-r11.2\stuff\sgrunt-keen5 1508M.jmd", 640, 400, "640 400 PointResize !720 400 PointResize")

Info

PointResize (640, 400)


c1 = TimeCodeFPS ("jmdtestcode.txt", 60001, 1000, false, 0.4, 0.9).ConvertToRGB32 ()
c2 = TimeCodeFPS ("jmdtestcode.txt", 60001, 1000, true, 0.4, 0.9)

StackVertical (c1, c2)

#StackHorizontal (SincResize (320, 200, -20, -20, 360, 240), last)

