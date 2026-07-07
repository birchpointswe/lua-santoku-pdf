local env = {

  name = "santoku-pdf",
  version = "0.0.7-1",
  variable_prefix = "TK_PDF",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.328-1",
  },

  test = {
    dependencies = {
      "santoku-fs >= 0.0.45-1",
    }
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {

  env = env,
}
