# Copyright 2009 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

vars = {
  # chromium.org and googlecode.com will redirect https URLs for directories
  # that don't end with a trailing slash to http. We therefore try to make sure
  # all https URLs include the trailing slash, but it's unclear if SVN actually
  # respects this.
  "chromium_git": "https://chromium.googlesource.com",
  # We don't include @ inside the revision here as is customary since
  # we want to pass this into a -D flag
  "chromium_revision_num": "256281",

  "optipng_src": "https://github.com/pagespeed/optipng.git",
  "optipng_revision": "d48f8e496e59ba6a9f90fecdd8f6f80685abb324",

  "giflib_src": "https://github.com/pagespeed/giflib.git",
  "giflib_revision": "cbbe2391f92b3fbee6c68216a1a99cbd32596e61",

  "icu_src": "https://github.com/pagespeed/icu.git",
  "icu_revision": "e8c51b17d1467b8a1927baf2215702c3a7682cc8",

  # TODO(cheesy): There's a perfectly fine zlib repo at
  # https://github.com/madler/zlib.git. We should copy the gyp file into our
  # repo and then switch to it. After that, we should delete the zlib
  # repo from pagespeed.
  "zlib_src": "https://github.com/pagespeed/zlib.git",
  "zlib_revision": "877ddb5c3793ff6f61e45d5f51a2399a7d4098ca",

  # Import libwebp 0.4.1 from the official repo.
  "libwebp_src": "https://chromium.googlesource.com/webm/libwebp.git",
  "libwebp_revision": "@8af2771813632e2007988c8df6ad7e68b28ad121",

  "modspdy_src": "https://github.com/pagespeed/mod_spdy.git",
  "modspdy_revision": "@818c05cef721862a36c1733e72e5e3591a856765",

  "serf_src": "https://git.apache.org/serf.git",
  "serf_revision": "@1.3.8",

  "apr_src": "git://git.apache.org/apr.git",
  "apr_revision": "@1.5.1",

  "aprutil_src": "git://git.apache.org/apr-util.git",
  "aprutil_revision": "@1.5.4",

  "apache_httpd_src":
    "git://git.apache.org/httpd.git",
  "apache_httpd_revision": "@2.2.29",

  "apache_httpd24_src":
    "git://git.apache.org/httpd.git",
  "apache_httpd24_revision": "@2.4.10",

  # The closure library doesn't have versioned releases so using head from
  # 2015-03-02. This should be updated periodically (monthly) to keep it in sync
  # with the closure compiler. The closure group only provides binary releases
  # of the compiler from head, and it's not worth it to try to build the
  # compiler ourselves.
  "closure_library": "https://github.com/google/closure-library.git",
  "closure_library_revision": "@2012c5372fdb02ce9531cf4b2561b05e3ce2ab39",

  "jsoncpp_src": "https://github.com/open-source-parsers/jsoncpp.git",
  "jsoncpp_revision": "@7165f6ac4c482e68475c9e1dac086f9e12fff0d0",

  "gflags_src_revision": "@e7390f9185c75f8d902c05ed7d20bb94eb914d0c",
  "gflags_revision": "@cc7e9a4b374ff7b6a1cae4d76161113ea985b624",

  "google_sparsehash_root":
    "https://github.com/google/sparsehash.git",
  "google_sparsehash_revision": "@6ff8809259d2408cb48ae4fa694e80b15b151af3",

  "gtest_src": "https://github.com/google/googletest.git",
  "gtest_revision": "@c99458533a9b4c743ed51537e25989ea55944908",

  "gmock_src": "https://github.com/google/googlemock.git",
  "gmock_revision": "@c440c8fafc6f60301197720617ce64028e09c79d",

  # Comment this out to disable HTTPS fetching via serf.  See also the
  # references in src/third_party/serf/serf.gyp.
  #
  # TODO(jmarantz): create an easy way to choose this option from the
  # 'gclient' command, without having to edit the gyp & DEPS files.
  #
  # BoringSSL commit picked from Chromium 52 Stable.
  "boringssl_src": "https://boringssl.googlesource.com/boringssl.git",
  "boringssl_git_revision": "@54092ffeaa80ed032a50b914f14e3aa41de763df",

  "domain_registry_provider_src":
     "https://github.com/pagespeed/domain-registry-provider.git",
  "domain_registry_provider_revision":
     "@e9b72eaef413335eb054a5982277cb2e42eaead7",

  "libpng_src": "https://github.com/glennrp/libpng.git",
  "libpng_revision": "@a36c4f3f165fb2dd1772603da7f996eb40326621",

  "proto_src": "https://github.com/google/protobuf.git",
  "protobuf_revision": "v3.0.0",
}

deps = {
  "src/third_party/giflib": Var("giflib_src") + "@" + Var("giflib_revision"),
  "src/third_party/icu": Var("icu_src") + "@" + Var("icu_revision"),
  "src/third_party/optipng": Var("optipng_src") + "@" + Var("optipng_revision"),
  "src/third_party/zlib": Var("zlib_src") + "@" + Var("zlib_revision"),

  "src/third_party/libjpeg_turbo/yasm/source/patched-yasm":
    Var("chromium_git") + "/chromium/deps/yasm/patched-yasm@7da28c6c7c6a1387217352ce02b31754deb54d2a",
  "src/third_party/libjpeg_turbo/src":
    Var("chromium_git") + "/chromium/deps/libjpeg_turbo/@7260e4d8b8e1e40b17f03fafdf1cd83296900f76",

  "src/testing":
    Var("chromium_git") + "/chromium/src/testing/@3207604f790d18c626e9dcb1a09874618c68844b",
  "src/testing/gtest": Var("gtest_src") + Var("gtest_revision"),
  "src/testing/gmock": Var("gmock_src") + Var("gmock_revision"),

  "src/third_party/apr/src":
    Var("apr_src") + Var("apr_revision"),

  "src/third_party/aprutil/src":
    Var("aprutil_src") + Var("aprutil_revision"),

  "src/third_party/httpd/src":
    Var("apache_httpd_src") + Var("apache_httpd_revision"),

  "src/third_party/httpd24/src":
    Var("apache_httpd24_src") + Var("apache_httpd24_revision"),

  "src/third_party/chromium/src/base":
    Var("chromium_git") + "/chromium/src/base@ccf3c2f324c4ae0d1aa878921b7c98f7deca5ee8",

  "src/third_party/chromium/src/build":
    Var("chromium_git") + "/chromium/src/build/@06b7bd9c7a8adb3708db8df4dc058de94f0d5554",

  # This revision is before headers got moved to main chromium repo.
  "src/third_party/chromium/src/googleurl":
    Var("chromium_git") + "/external/google-url@405b6e1798f88e85291820b30344723512e0c38f",

  "src/third_party/closure_library":
    Var("closure_library") + Var("closure_library_revision"),

  "src/third_party/gflags":
    Var('chromium_git') + '/external/webrtc/trunk/third_party/gflags' +
    Var('gflags_revision'),
  "src/third_party/gflags/src":
    Var('chromium_git') + '/external/gflags/src' + Var("gflags_src_revision"),

  "src/third_party/google-sparsehash":
    Var("google_sparsehash_root") + Var("google_sparsehash_revision"),

  "src/third_party/protobuf/src":
    Var("proto_src") + '@' + Var("protobuf_revision"),

  # Json cpp.
  "src/third_party/jsoncpp/src":
    Var("jsoncpp_src") + Var("jsoncpp_revision"),

  # Serf
  "src/third_party/serf/src": Var("serf_src") + Var("serf_revision"),

  "src/third_party/mod_spdy/src": Var("modspdy_src") + Var("modspdy_revision"),

  "src/third_party/libwebp": Var("libwebp_src") + Var("libwebp_revision"),

  "src/tools/clang":
    Var("chromium_git") + "/chromium/src/tools/clang/@bf272f7b05896b9a18de8497383f8b873a86cfbc",

  # This is the same commit as the version from svn included from chromium_deps,
  # but the svn is down, so we take it from chromium-git.
  "src/tools/gyp":
    Var("chromium_git") + "/external/gyp@" + "0fb31294ae844bbf83eba05876b7a241b66f1e99",

  "src/third_party/modp_b64":
    Var("chromium_git") + "/chromium/src/third_party/modp_b64/@aae60754fa997799e8037f5e8ca1f56d58df763d",

  # RE2.
  # This is release 2016-02-01, the latest that works w/o C++11 (which we don't
  # require on this branch).
  "src/third_party/re2/src":
    "https://github.com/google/re2.git/@7436831ef39b89b3a2ea50be91cf09732a900239",

  # Comment to disable HTTPS fetching via serf.  See also the
  # references in src/third_party/serf/serf.gyp.
  "src/third_party/boringssl/src":
    Var("boringssl_src") + Var("boringssl_git_revision"),

  # Domain Registry Provider gives us the Public Suffix List.
  "src/third_party/domain_registry_provider":
    Var("domain_registry_provider_src") +
        Var("domain_registry_provider_revision"),

  "src/third_party/libpng/src": Var("libpng_src") + Var("libpng_revision")
}


include_rules = [
  # Everybody can use some things.
  "+base",
  "+build",
]


# checkdeps.py shouldn't check include paths for files in these dirs:
skip_child_includes = [
   "testing",
]


hooks = [
  {
    # Pull clang on mac. If nothing changed, or on non-mac platforms, this takes
    # zero seconds to run. If something changed, it downloads a prebuilt clang,
    # which takes ~20s, but clang speeds up builds by more than 20s.
    "pattern": ".",
    "action": ["python", "src/tools/clang/scripts/update.py", "--mac-only"],
  },
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "src/build/gyp_chromium",
               "-Dchromium_revision=" + Var("chromium_revision_num")],
  },
  {
    "pattern": ".",
    "action": ["src/third_party/closure/download.sh"],
  },
]
