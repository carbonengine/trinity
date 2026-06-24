// Copyright © 2026 CCP ehf.

package Windows

import jetbrains.buildServer.configs.kotlin.DslContext
import jetbrains.buildServer.configs.kotlin.Project
import jetbrains.buildServer.configs.kotlin.*
import jetbrains.buildServer.configs.kotlin.buildFeatures.PullRequests
import jetbrains.buildServer.configs.kotlin.buildFeatures.XmlReport
import jetbrains.buildServer.configs.kotlin.buildFeatures.commitStatusPublisher
import jetbrains.buildServer.configs.kotlin.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.buildFeatures.nuGetFeedCredentials
import jetbrains.buildServer.configs.kotlin.buildFeatures.perfmon
import jetbrains.buildServer.configs.kotlin.buildFeatures.pullRequests
import jetbrains.buildServer.configs.kotlin.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.buildFeatures.xmlReport
import jetbrains.buildServer.configs.kotlin.buildSteps.exec
import jetbrains.buildServer.configs.kotlin.buildSteps.powerShell
import jetbrains.buildServer.configs.kotlin.buildSteps.python
import jetbrains.buildServer.configs.kotlin.buildSteps.script
import jetbrains.buildServer.configs.kotlin.triggers.vcs
import jetbrains.buildServer.configs.kotlin.vcs.GitVcsRoot
import jetbrains.buildServer.configs.kotlin.buildFeatures.provideAwsCredentials

val Debug = CarbonBuildWindows("Debug Windows", "Debug", "x64-windows-debug")
val Internal = CarbonBuildWindows("Internal Windows", "Internal", "x64-windows-internal")
val TrinityDev = CarbonBuildWindows("TrinityDev Windows", "TrinityDev", "x64-windows-trinitydev")
val Release = CarbonBuildWindows("Release Windows", "Release", "x64-windows-release")

object Project : Project({
    id("Windows")
    name = "Windows"

    buildType(Debug)
    buildType(Internal)
    buildType(TrinityDev)
    buildType(Release)
})


class CarbonBuildWindows(buildName: String, configType: String, preset: String) : BuildType({
    id(buildName.toId())
    this.name = buildName

    artifactRules = "%env.CMAKE_INSTALL_PREFIX% => artifact.zip"

    params {
        param("env.GIT_TAG_HASH_OVERRIDE", "")
        param("github_checkout_folder", "github")
        param("env.CTEST_JUNIT_OUTPUT_FILE", "ctest_results.xml")
        select("env.VISUAL_STUDIO_PLATFORM_TOOLSET", "v141", label = "Visual Studio Platform Toolset", description = "Specify the toolset for the build. e.g. v141 or v143.",
                options = listOf("v141 (2017)" to "v141", "v143 (2022)" to "v143"))
        param("VS_DEV_BAT_SWITCHES", "-arch=x64")
        param("env.VSDEV_BAT_PATH", "%%ProgramFiles(x86)%%/Microsoft Visual Studio/2017/BuildTools/Common7/Tools/vsdevcmd.bat")
        param("env.CMAKE_BUILD_TARGETS", "all")
        param("env.CMAKE_INSTALL_PREFIX", ".build-artifact")
        param("env.CMAKE_CONFIG_TYPE", configType)
        param("env.SENTRY_PROJECT", "exefile-crashes")
        param("env.CMAKE_BUILD_FOLDER", ".cmake-build-%build.number%")
        param("env.CMAKE_GENERATOR", "Ninja Multi-Config")
        param("env.GIT_TAG_HASH", "")
        param("env.EXECUTABLE_FILENAMES_MATCH", "")
        param("teamcity.vcsTrigger.runBuildInNewEmptyBranch", "true")
        param("env.CMAKE_PRESET", preset)
        param("env.VCPKG_BINARY_SOURCES", "clear;x-aws,s3://vcpkg-binary-cache-static/cache/,readwrite")
        param("env.X_VCPKG_REGISTRIES_CACHE", "%teamcity.build.checkoutDir%/%github_checkout_folder%/regcache")
        param("env.CMAKE_BUILD_PARALLEL_LEVEL", "8")
        param("env.CTEST_PARALLEL_LEVEL", "8")
    }

    vcs {
        root(DslContext.settingsRootId, "+:. => %github_checkout_folder%")
        root(AbsoluteId("CarbonPipelineTools"), "+:. => carbon_pipeline_tools")
        cleanCheckout = true
    }

    steps {
        exec {
            name = "Create VCPKG registrycache location"
            workingDir = "%teamcity.build.checkoutDir%/%github_checkout_folder%"
            path = "mkdir"
            arguments = "regcache"
        }
        exec {
            name = "(Windows) Get Git Tag/Hash"
            workingDir = "%teamcity.build.checkoutDir%/%github_checkout_folder%"
            path = "python"
            arguments = "%teamcity.build.checkoutDir%/carbon_pipeline_tools/carbon/GetGitTagAndOrHash.py"
        }
        script {
            name = "(Windows) Bootstrap Build environment"
            workingDir = "%teamcity.agent.work.dir%"
            scriptContent = """
                REM unfortunately ninja does not find the VS environment otherwise
                REM NB: the exported PATH also contains the location where we installed sentry-cli, e.g. teamcity.agent.work.dir
                call "%env.VSDEV_BAT_PATH%" %VS_DEV_BAT_SWITCHES%
                echo ##teamcity[setParameter name='env.INCLUDE' value='%%INCLUDE%%']
                echo ##teamcity[setParameter name='env.LIB' value='%%LIB%%']
                echo ##teamcity[setParameter name='env.LIBPATH' value='%%LIBPATH%%']
                echo ##teamcity[setParameter name='env.PATH' value='%teamcity.agent.work.dir%;%%PATH%%']
            """.trimIndent()
        }
        exec {
            name = "Configure"
            path = "cmake"
            arguments = "--preset %env.CMAKE_PRESET% -S %teamcity.build.checkoutDir%/%github_checkout_folder% -B %env.CMAKE_BUILD_FOLDER% -DINSTALL_TO_MONOLITH=ON -DCMAKE_INSTALL_PREFIX=%env.CMAKE_INSTALL_PREFIX% -DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root=%teamcity.build.checkoutDir%/%github_checkout_folder%/buildtrees -DBUILD_DX11=ON -DBUILD_DX12=ON -DBUILD_SHADER_COMPILER=ON -DWITH_GRANNY=ON"
        }
        exec {
            name = "Build"
            path = "cmake"
            arguments = "--build %env.CMAKE_BUILD_FOLDER% --config %env.CMAKE_CONFIG_TYPE% --target %env.CMAKE_BUILD_TARGETS%"
        }
        exec {
            name = "Run Tests"
            workingDir = "%env.CMAKE_BUILD_FOLDER%"
            path = "ctest"
            arguments = "-C %env.CMAKE_CONFIG_TYPE% -V --output-on-failure --output-junit %env.CTEST_JUNIT_OUTPUT_FILE%"
        }
        exec {
            name = "Package artifact"
            path = "cmake"
            arguments = "--install %env.CMAKE_BUILD_FOLDER% --config %env.CMAKE_CONFIG_TYPE%"
        }
        exec {
            name = "Upload symbols to sentry"
            path = "sentry-cli"
            arguments = "upload-dif --wait %env.CMAKE_BUILD_FOLDER%"
            param("script.content", """
                #!/usr/bin/env bash -eu
                filesWithSymbols = (
                  # insert your binary files with symbols here!
                )
            """.trimIndent())
        }
        script {
            name = "(Windows) CMD Upload debug symbols to internal symbol server"
            scriptContent = """
                @echo off
                (
                echo ${'$'}User = "%DOMAIN_USER%"
                echo ${'$'}Password = ConvertTo-SecureString -String "%DOMAIN_USER_PASSWORD%" -AsPlainText -Force
                echo ${'$'}Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList ${'$'}User, ${'$'}Password
                echo New-PSDrive -Name "symbols" -PSProvider FileSystem -Root ${'$'}Env:TC_SYMBOL_STORE_PATH -Credential ${'$'}Credential
                echo Write-Host "##teamcity[progressMessage 'Storing symbols']"
                echo ${'$'}symstoreFlags = ^@^("add","/compress","/t", "CCP Games", "/c", "TeamCity %build.number%", "/s", "${'$'}Env:TC_SYMBOL_STORE_PATH", "/o", "/r",  "/f", "%env.CMAKE_BUILD_FOLDER%"^) 
                echo ^& ${'$'}Env:TC_SYMSTORE_PATH ${'$'}symstoreFlags ^| Tee-Object -file symstore.txt
                echo ${'$'}stored = get-content symstore.txt ^| Select-String "^SYMSTORE: Number of files stored = (.*)${'$'}"
                echo ${'$'}stored = ${'$'}stored.Matches.Groups[1].Value
                echo ${'$'}errors = get-content symstore.txt ^| Select-String "^SYMSTORE: Number of errors = (.*)${'$'}"
                echo ${'$'}errors = ${'$'}errors.Matches.Groups[1].Value
                echo ${'$'}ignored = get-content symstore.txt ^| Select-String "^SYMSTORE: Number of files ignored = (.*)${'$'}"
                echo ${'$'}ignored = ${'$'}ignored.Matches.Groups[1].Value
                echo Write-Host "##teamcity[buildStatus text='Stored: ${'$'}stored, Errors: ${'$'}errors, Ignored: ${'$'}ignored']"
                ) > file.ps1
                powershell -File file.ps1
            """.trimIndent()
        }
        powerShell {
            name = "(Windows) Upload debug symbols to internal symbol server"
            enabled = false

            conditions {
                startsWith("teamcity.agent.jvm.os.name", "Windows")
            }
            scriptMode = script {
                content = """
                    ${'$'}User = "%DOMAIN_USER%"
                    ${'$'}Password = ConvertTo-SecureString -String "%DOMAIN_USER_PASSWORD%" -AsPlainText -Force
                    ${'$'}Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList ${'$'}User, ${'$'}Password
                    New-PSDrive -Name "symbols" -PSProvider FileSystem -Root ${'$'}Env:TC_SYMBOL_STORE_PATH -Credential ${'$'}Credential
                    
                    Write-Host "##teamcity[progressMessage 'Storing symbols']"
                    ${'$'}symstoreFlags = @("add",
                                     "/compress",
                                     "/t", "CCP Games", # Product Name
                                     "/c", "TeamCity %build.number%", #Comment
                                     "/s", "${'$'}Env:TC_SYMBOL_STORE_PATH", # Store destination
                                     "/o", # Verbose
                                     "/r", # Recursive
                                     "/f", "%env.CMAKE_BUILD_FOLDER%") # source folder
                    & ${'$'}Env:TC_SYMSTORE_PATH ${'$'}symstoreFlags | Tee-Object -file symstore.txt
                    
                    ${'$'}stored = get-content symstore.txt | Select-String "^SYMSTORE: Number of files stored = (.*)${'$'}"
                    ${'$'}stored = ${'$'}stored.Matches.Groups[1].Value
                    
                    ${'$'}errors = get-content symstore.txt | Select-String "^SYMSTORE: Number of errors = (.*)${'$'}"
                    ${'$'}errors = ${'$'}errors.Matches.Groups[1].Value
                    
                    ${'$'}ignored = get-content symstore.txt | Select-String "^SYMSTORE: Number of files ignored = (.*)${'$'}"
                    ${'$'}ignored = ${'$'}ignored.Matches.Groups[1].Value
                    
                    Write-Host "##teamcity[buildStatus text='Stored: ${'$'}stored, Errors: ${'$'}errors, Ignored: ${'$'}ignored']"
                """.trimIndent()
            }
        }
    }

    triggers {
        vcs {
            triggerRules = "+:root=${DslContext.settingsRootId.id}:."
             branchFilter = """
                            +:<default>
                            +pr:*
                        """.trimIndent()
        }
    }

    features {
        pullRequests {
            vcsRootExtId = "${DslContext.settingsRootId.id}"
            provider = github {
                authType = token {
                    token = "%GITHUB_CARBON_PAT%"
                }
                // Constrain PR triggers to compatible refs so as to avoid erroneous triggers
                filterTargetBranch = """
                    +:refs/heads/main
                    +:refs/heads/release/*.x
                    -:refs/heads/release/1.x
                """.trimIndent()
                filterAuthorRole = PullRequests.GitHubRoleFilter.MEMBER
            }
        }
        commitStatusPublisher {
            publisher = github {
                githubUrl = "https://api.github.com"
                authType = personalToken {
                    token = "%GITHUB_CARBON_PAT%"
                }
            }
        }
        xmlReport {
            reportType = XmlReport.XmlReportType.JUNIT
            rules = "+:%env.CMAKE_BUILD_FOLDER%/%env.CTEST_JUNIT_OUTPUT_FILE%"
            verbose = true
        }
        perfmon {
        }
        freeDiskSpace {
            requiredSpace = "10gb"
            failBuild = true
        }
        sshAgent {
            teamcitySshKey = "ccpgames-evetech GitHub"
        }
        provideAwsCredentials {
            awsConnectionId = "Carbon_AwsVcpkgBinaryCacheServiceAccount"
        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server")
    }
})
