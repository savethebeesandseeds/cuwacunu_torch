/*
  tsodao.dsl
  ==========
  First TSODAO component:
    Public/private surface guard for authored material that must stay local in
    plaintext while the public repo tracks only an encrypted archive.

  Current mapping:
    - hidden_root points at src/config/instructions/optim
    - hidden_archive is the tracked encrypted backup for that surface
    - public_keep_path stays visible in the public repo so collaborators know
      the surface exists and how to restore it
    - local_state_path stores local TSODAO bookkeeping outside the repo
*/

visibility_mode[recipient|symmetric|manual|disabled]:str = recipient
hidden_root:path = /cuwacunu/src/config/instructions/optim
hidden_archive:path = /cuwacunu/src/config/instructions/optim.tar.gpg
public_keep_path:path = /cuwacunu/src/config/instructions/optim/README.md
local_state_path:path = /cuwacunu/.runtime/.tsodao/optim.state
gpg_recipient:str = 2D42B6A4B56367BCDB6ECAB9260A0C3A37DB9651
