/*
  default.hero.human.dsl
  Human Hero attests and answers pending MARSHAL session requests.
  If operator_id is left as CHANGE_ME_OPERATOR, the first Human Hero use
  auto-initializes it to <user>@<hostname> and persists that local identity.
  operator_signing_ssh_identity must match the public key registered for
  operator_id in default.hero.marshal.dsl human_operator_identities.
*/
marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal_mcp
operator_id:str = root@c98d8db10d76 # managed by setup_human_operator.sh
operator_signing_ssh_identity:path = ../../secrets/real/human_operator_ed25519 # must match operator_id in human_operator_identities
