local env = {

  name = "santoku-pdf",
  version = "0.0.3-1",
  variable_prefix = "TK_PDF",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
  },

  test = {
    dependencies = {
      "santoku >= 0.0.203-1",
      "santoku-fs >= 0.0.30-1",
      "luacov >= 0.15.0-1",
    }
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  type = "lib",
  env = env,
}
