#!/usr/bin/env python3
import os
import stat
import zipfile
import shutil

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_VSIX_110 = os.path.join(BASE_DIR, "cicekliscript-1.1.0.vsix")
OUT_VSIX_100 = os.path.join(BASE_DIR, "cicekliscript-1.0.0.vsix")

CONTENT_TYPES_XML = """<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".js" ContentType="application/javascript"/>
  <Default Extension=".json" ContentType="application/json"/>
  <Default Extension=".md" ContentType="text/markdown"/>
  <Default Extension=".png" ContentType="image/png"/>
  <Default Extension=".svg" ContentType="image/svg+xml"/>
  <Default Extension=".txt" ContentType="text/plain"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
  <Default Extension=".exe" ContentType="application/vnd.microsoft.portable-executable"/>
  <Default Extension="" ContentType="application/octet-stream"/>
</Types>
"""

VSIX_MANIFEST = """<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="cicekliscript" Version="1.1.0" Publisher="cicekliscript" />
    <DisplayName>CicekliScript</DisplayName>
    <Description xml:space="preserve">CicekliScript dili (.ccl) — Dahili yerleşik C11 derleyicisi, sözdizimi renklendirme, snippet'lar ve tek tıkla çalıştırma.</Description>
    <Tags>cicekliscript,ccl,cicekli,metescript,ms,compiler,cuda,programming language,python-like,c-based,theme,icon-theme,snippet,keybindings,CicekliScript,MeteScript,__ext_ccl,__ext_cicekli,__ext_ms,__ext_metescript</Tags>
    <Categories>Programming Languages,Snippets,Themes</Categories>
    <GalleryFlags>Public</GalleryFlags>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="^1.70.0" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionDependencies" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionPack" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionKind" Value="workspace" />
      <Property Id="Microsoft.VisualStudio.Code.LocalizedLanguages" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.EnabledApiProposals" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.ExecutesCode" Value="true" />
      <Property Id="Microsoft.VisualStudio.Services.Links.Source" Value="https://github.com/drkkahraman/CicekliScript.git" />
      <Property Id="Microsoft.VisualStudio.Services.Links.Getstarted" Value="https://drkkahraman.github.io/CicekliScript/" />
      <Property Id="Microsoft.VisualStudio.Services.Links.GitHub" Value="https://github.com/drkkahraman/CicekliScript.git" />
      <Property Id="Microsoft.VisualStudio.Services.Links.Support" Value="https://github.com/drkkahraman/CicekliScript/issues" />
      <Property Id="Microsoft.VisualStudio.Services.Links.Learn" Value="https://drkkahraman.github.io/CicekliScript/" />
      <Property Id="Microsoft.VisualStudio.Services.GitHubFlavoredMarkdown" Value="true" />
      <Property Id="Microsoft.VisualStudio.Services.Content.Pricing" Value="Free"/>
    </Properties>
    <License>extension/LICENSE.txt</License>
    <Icon>extension/icons/logo.png</Icon>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code"/>
  </Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true" />
    <Asset Type="Microsoft.VisualStudio.Services.Content.Details" Path="extension/readme.md" Addressable="true" />
    <Asset Type="Microsoft.VisualStudio.Services.Content.License" Path="extension/LICENSE.txt" Addressable="true" />
    <Asset Type="Microsoft.VisualStudio.Services.Icons.Default" Path="extension/icons/logo.png" Addressable="true" />
  </Assets>
</PackageManifest>
"""

def build_vsix():
    # Make sure bin/cicekliscript exists
    bin_path = os.path.join(BASE_DIR, "bin", "cicekliscript")
    root_bin = os.path.abspath(os.path.join(BASE_DIR, "..", "bin", "cicekliscript"))
    if not os.path.exists(bin_path):
        os.makedirs(os.path.join(BASE_DIR, "bin"), exist_ok=True)
        if os.path.exists(root_bin):
            shutil.copy2(root_bin, bin_path)
            os.system(f"strip -s {bin_path}")
        else:
            raise FileNotFoundError(f"Binary not found: {bin_path}")
    os.chmod(bin_path, 0o755)

    ccl_link = os.path.join(BASE_DIR, "bin", "ccl")
    if not os.path.exists(ccl_link):
        try:
            os.symlink("cicekliscript", ccl_link)
        except OSError:
            pass

    # Build files list
    include_dirs = ["bin", "icons", "snippets", "syntaxes"]
    include_files = [
        "extension.js",
        "package.json",
        "language-configuration.json",
        "LICENSE",
        "README.md",
    ]

    all_files = []
    for f in include_files:
        fp = os.path.join(BASE_DIR, f)
        if os.path.exists(fp):
            arc = "extension/" + ("LICENSE.txt" if f == "LICENSE" else ("readme.md" if f == "README.md" else f))
            all_files.append((fp, arc))

    for d in include_dirs:
        dp = os.path.join(BASE_DIR, d)
        for root, _, files in os.walk(dp):
            for file in files:
                full_path = os.path.join(root, file)
                rel = os.path.relpath(full_path, BASE_DIR)
                arc = "extension/" + rel.replace(os.sep, "/")
                all_files.append((full_path, arc))

    print(f"Paketleniyor: {len(all_files)} dosya...")

    for target_path in [OUT_VSIX_110, OUT_VSIX_100]:
        with zipfile.ZipFile(target_path, "w", zipfile.ZIP_DEFLATED) as z:
            # 1. [Content_Types].xml
            z.writestr("[Content_Types].xml", CONTENT_TYPES_XML.strip())
            # 2. extension.vsixmanifest
            z.writestr("extension.vsixmanifest", VSIX_MANIFEST.strip())

            # 3. Extension files
            for full_path, arcname in all_files:
                is_exec = False
                if "bin/cicekliscript" in arcname or "bin/ccl" in arcname or arcname.endswith(".exe"):
                    is_exec = True

                with open(full_path, "rb") as fh:
                    data = fh.read()

                zinfo = zipfile.ZipInfo(arcname)
                mode = 0o755 if is_exec else 0o644
                zinfo.external_attr = (stat.S_IFREG | mode) << 16
                z.writestr(zinfo, data)

        size_kb = round(os.path.getsize(target_path) / 1024, 1)
        print(f"✓ VSIX başarıyla üretildi: {target_path} ({size_kb} KB)")

if __name__ == "__main__":
    build_vsix()
