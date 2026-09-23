# win-svchost-service-injector
DLL service loader that uses Service Group Injection and Registry-Based Service Registration to execute a DLL inside a native Windows svchost.exe process context.  The implementation includes both standard SCM API usage and direct, low-level registry manipulation to establish service parameters and execute payloads.
