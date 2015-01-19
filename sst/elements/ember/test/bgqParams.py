
debug = 0

networkParams = {
    "topology" : "merlin." + "torus",
    "packetSize" : "2048B",
    "link_bw" : "1.77GB/s",
    "flitSize" : "8B",
    "buffer_size" : "14KB",
}

nicParams = {
    "debug" : 0,
    "verboseLevel": 1,
    "module" : "merlin.linkcontrol",
    "topology" : networkParams['topology'], 
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "buffer_size" : networkParams['buffer_size'],
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
}

emberParams = {
    "os.module"    : "firefly.hades",
    "os.name"      : "hermesParams",
    "api.0.module" : "firefly.hadesMP",
    "verbose" : 0,
}

hermesParams = {
    "hermesParams.debug" : 0,
    "hermesParams.verboseLevel" : 1,
    "hermesParams.nicModule" : "firefly.VirtNic",
    "hermesParams.nicParams.debug" : 0,
    "hermesParams.nicParams.debugLevel" : 1 ,
    "hermesParams.policy" : "adjacent",
    "hermesParams.functionSM.defaultEnterLatency" : 30000,
    "hermesParams.functionSM.defaultReturnLatency" : 30000,
    "hermesParams.functionSM.defaultDebug" : 0,
    "hermesParams.functionSM.defaultVerbose" : 2,
    "hermesParams.ctrlMsg.debug" : 0,
    "hermesParams.ctrlMsg.verboseLevel" : 2,
    "hermesParams.ctrlMsg.shortMsgLength" : 5000,
    "hermesParams.ctrlMsg.matchDelay_ns" : 260,

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-64:1480ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.1" : "65-256:2480ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.2" : "257-:2980ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-64:100ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.1" : "65-256:1100ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.2" : "257-:1600ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.txMemcpyModParams.base" : "120ns",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-:156ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.rxMemcpyModParams.base" : "120ns",
    "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-:156ps",

    "hermesParams.ctrlMsg.txNicDelay_ns" : 0,
    "hermesParams.ctrlMsg.rxNicDelay_ns" : 0,

    "hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 700,
    "hermesParams.ctrlMsg.recvReqFiniDelay_ns" : 0,

    "hermesParams.ctrlMsg.sendAckDelay_ns" : 0,

    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 1500,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 0,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
