# Read-only Files browser

The PID 1 desktop uses existing DIRECTORY_OPEN / DIRECTORY_READ, FILE_OPEN /
FILE_READ and HANDLE_CLOSE syscalls to browse real VFS directories and inspect
file content. It adds no kernel ABI or writes to the selected volume.

| Key in Files | Result |
| --- | --- |
| Up / Down | Select an entry; cross page boundaries at the first/last row |
| Page Up / Page Down | Previous/next page, selecting its first entry |
| Enter | Enter a selected directory or preview a regular file |
| Backspace | Parent directory; from a preview, return to its list |
| Escape | Close a preview; from a list, return Home |
| B / D | Return to the `/boot` / `/disk` root |
| R | Reload the current directory and close any preview |

Only the initial key-down acts; repeats, releases and Ctrl/Alt/Meta chords are
ignored. Home/Files/Settings navigation remains available. Re-entering Files
reloads its remembered directory. A root key also resets a nested location when
that root is already selected, and resets a later page to the beginning. A
missing volume shows the existing retry message.

The list holds twelve entries at a time, without limiting the directory to
twelve entries. Each page is read from a newly opened directory handle, skipping
earlier entries, validating copied metadata (including skipped entries), and
reading one lookahead entry to distinguish a full final page from another page.
The handle is closed before display/input resumes. The heading shows the ordinal
range, for example `Files 13-24+`; `+` means a following page exists. Up at the
first row selects the last entry of the previous page; Down at the last row
selects the first entry of the next page. Directory/root/parent transitions
start on the first page. Closing a preview preserves its page and selection.

Failed or vanished page transitions preserve the visible path, entries, offset
and selection, and display the recovery notice. R refreshes the current page;
if that page is now empty, it reloads the first page. Enumeration is an ordinal
snapshot, not a persistent cursor: concurrent directory changes may move entries
between pages, and R obtains a fresh snapshot. Page loads rescan from the start,
so large directories can take longer despite constant snapshot memory. Page
offset arithmetic is checked against the 32-bit entry index before any open.

Locations use at most 255 bytes. Child names are length-delimited, up to 63
bytes; NUL/control bytes, slash, backslash and lexical `.`/`..` are rejected
before any filesystem call. Backspace stays inside the selected root. Entry
names are used intact for lookup; clipped/sanitized display labels are never
used as paths. A failed directory transition leaves the previous path, list
and selection intact and displays a recoverable error. Device entries cannot
be opened by this browser.

Files are opened with READ access only. The preview reads up to 512 bytes in
chunks no larger than 256 bytes, continuing after short reads. One extra byte
distinguishes an exactly 512-byte file from a longer file. EOF, empty files,
open/read failures and close failures have explicit handling. Every successful
open gets one close attempt; failed reads discard partial preview content and
leave the list available. File metadata is not trusted to predict EOF.

The preview has at most twelve rows of 48 printable ASCII cells. CRLF/LF/CR
start a new row, Tab advances to a four-column stop, and other control or
non-ASCII bytes become `.`. Text wraps at the row edge. A truncation notice
appears when either the byte or visible-row bound omits content. Empty files
show an explicit empty message. Filenames/path labels use separate inert ASCII
sanitization and clipping, so neither binary data nor directory names inject
renderer control sequences.

Host tests exercise nested transitions, parent/root limits, rejected names and
devices, selection/repeats (including actual PS/2 arrow scan bytes through the
kernel decoder), short reads, exact-boundary truncation, empty and
binary content, failures before/after partial reads and close attempts. Existing
model/view tests and image-checker tests remain regression coverage. QEMU boots
the ordinary build, visits `/disk/SAVES/WORLDS`, reads actual FAT32 files and a
boot ELF, closes previews, and returns through lists/root/Home. The independent
checker compares 26 completed screenshots at every RGB pixel, using the real
fixture bytes and ELF artifacts as data. The entire disk SHA-256 must remain
unchanged. The normal shell ISO is retained alongside the CI screen artifacts.

An additional QEMU boot uses a real 27-file FAT32 directory spanning multiple
clusters. Page Down/Up and arrow boundary transitions visit all three pages,
preview files 12, 13 and 25, return to the same selections, refresh a later page,
and return to the parent/root/Home. The independent checker compares 23 complete
screens and the entire disk SHA-256 remains unchanged. Its tested ISO, disk image,
seed files, checksum, logs and screen captures are retained together as CI
artifacts. Host coverage includes exact 12/13-entry boundaries, directory shrink,
skipped-entry corruption, read/close failures, preview identity and offset overflow.

The ordinary bootstrap directory probe accepts additional regular files and
directories after its `RUNTIME.TXT` seed, validates their type/name bounds and
reads through EOF before closing the handle. Its safety limit is 4096 additional
entries. Storage durability/rename/clean-unmount proof builds retain their exact
fixture checks. The browser QEMU check requires the additional-entry marker
before directory completion and shell readiness, so a multi-entry root cannot
silently regress to a boot-time stall.

This is still a bounded browser: twelve entries are visible per page, there is
no continuous scrolling, search, file editing, launching, Unicode filename display or
Unicode text layout, and only a file's beginning can be inspected. Settings
persistence, compositor/window support, networking/JVM/Minecraft remain separate
unfinished foundations.
