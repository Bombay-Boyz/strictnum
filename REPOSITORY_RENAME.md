# Rename the hosted repository to strictnum

The source tree, CMake project, package target, header and documentation use
`strictnum`. Renaming a hosted Git repository is a separate operation and is
not performed by a source patch.

1. On the hosting service, open the repository's **Settings → General** page
   and rename the repository to `strictnum`.
2. Update the local remote URL using the actual owner and transport you use:

   ```sh
   git remote set-url origin <new-strictnum-repository-url>
   git remote -v
   git ls-remote origin HEAD
   ```

   For GitHub SSH, the URL shape is
   `git@github.com:OWNER/strictnum.git`; replace `OWNER` with the repository's
   real owner. Do not copy a guessed owner or URL.
3. Check any README badges, release links, package metadata and external
   references that point to the former repository URL.

The uploaded source archive contained no configured Git remote and no commit,
so the owner and hosted repository could not be established from that archive.
