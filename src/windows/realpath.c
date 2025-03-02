WCHAR* realpath (const WCHAR*__restrict path, WCHAR*__restrict resolved_path) {
  return _wfullpath(NULL, path, 0);
}
bool winIsAbsolute(const char* path) { // TODO something more proper
  return *path && path[1]==':' && (!path[2] || path[2]=='/' || path[2]=='\\');
}
