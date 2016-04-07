
USN Journal
-----------

Read the links in the wikipedia page

https://en.wikipedia.org/wiki/USN_Journal

[Keeping an Eye on Your NTFS Drives: the Windows 2000 Change Journal Explained](
https://msdn.microsoft.com/en-us/library/bb742450.aspx )

http://www.codeproject.com/Articles/11594/Eyes-on-NTFS

http://az4n6.blogspot.com/2015/03/usn-journal-where-have-you-been-all-my.html

This one is binary only

https://github.com/jschicht/ExtractUsnJrnl

https://ejrh.wordpress.com/2012/07/06/using-the-ntfs-journal-for-backups/
http://www.cgsecurity.org/wiki/TestDisk
http://stackoverflow.com/questions/4172750/change-journal-for-blocks-in-windowsntfs


VSS
----

Wikipedia page describing the interface with a bunch of info to read.

https://en.wikipedia.org/wiki/Shadow_Copy

GUIDs

 * http://stackoverflow.com/questions/2237094/list-of-well-known-vss-writer-guids
 * http://stackoverflow.com/questions/17377593/list-of-well-known-guids

https://social.technet.microsoft.com/Forums/windows/en-US/19a3e3f8-5651-4868-90f9-0ca954d90974/diskshadow-for-windows-7-or-vssadmin?forum=w7itproinstall

http://microsoft.public.win32.programmer.kernel.narkive.com/2qwNWNxp/enumerating-the-usn-journal

 * http://www.osronline.com/showThread.CFM?link=200408
 * http://www.osronline.com/showThread.CFM?link=215910

### vscsc

This looks like the source for the `vshadow.exe` command.

https://sourceforge.net/p/vscsc/code/HEAD/tree/
https://sourceforge.net/projects/vscsc/

### HoboCopy

Based on RoboCopy

https://candera.github.io/hobocopy/
https://github.com/candera/hobocopy

### AlphaVSS

This one is C# wrappers only

https://github.com/alphaleonis/AlphaVSS

### MSDN articles

https://social.msdn.microsoft.com/Forums/en-US/2fe9bfd5-b449-4033-ae78-7353ef747a35/ivssbackupcomponentsquery-fails-with-undocumented-error-invalid-function-0x1?forum=windowssdk

 * [Volume Shadow Copy API Interfaces]( https://msdn.microsoft.com/en-us/library/aa384645(v=vs.85).aspx )
 * [IVssSnapshotMgmt interface]( https://msdn.microsoft.com/en-us/library/aa384284(v=vs.85).aspx )
 * [`VSS_SNAPSHOT_PROP` structure]( https://msdn.microsoft.com/en-us/library/aa384972(v=vs.85).aspx )


NTFS
----

http://www.codeproject.com/Articles/81456/An-NTFS-Parser-Lib

