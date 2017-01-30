tar -xvzf $HOST_GUEST_SCRIPT_DIR/proposal.tgz
cd proposal
make
evince proposal.pdf &
sleep 10
killall evince
cd ..
rm -rf proposal

