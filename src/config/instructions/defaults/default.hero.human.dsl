/*
  default.hero.human.dsl
  Human Hero attests and answers pending SUPER loop requests.
  If operator_id is left as CHANGE_ME_OPERATOR, the first Human Hero use
  auto-initializes it to <user>@<hostname> and persists that local identity.
  operator_signing_ssh_identity must match the public key registered for
  operator_id in default.hero.super.dsl human_operator_identities.
*/
super_hero_binary:path = /cuwacunu/.build/hero/hero_super_mcp
operator_id:str = root@c98d8db10d76 # managed by setup_human_operator.sh
operator_signing_ssh_identity:path = ../../secrets/real/human_operator_ed25519 # must match operator_id in human_operator_identities
