// Copyright © 2026 CCP ehf.

package MacOS

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

val arm64_Debug = CarbonBuildMacOS("Debug MacOS arm64", "Debug", "arm64-osx-debug", "aarch64")
val arm64_Internal = CarbonBuildMacOS("Internal MacOS arm64", "Internal", "arm64-osx-internal", "aarch64")
val arm64_TrinityDev = CarbonBuildMacOS("TrinityDev MacOS arm64", "TrinityDev", "arm64-osx-trinitydev", "aarch64")
val arm64_Release = CarbonBuildMacOS("Release MacOS arm64", "Release", "arm64-osx-release", "aarch64")

val x64_Debug = CarbonBuildMacOS("Debug MacOS x64", "Debug", "x64-osx-debug", "x86_64")
val x64_Internal = CarbonBuildMacOS("Internal MacOS x64", "Internal", "x64-osx-internal", "x86_64")
val x64_TrinityDev = CarbonBuildMacOS("TrinityDev MacOS x64", "TrinityDev", "x64-osx-trinitydev", "x86_64")
val x64_Release = CarbonBuildMacOS("Release MacOS x64", "Release", "x64-osx-release", "x86_64")

object Project : Project({
    id("MacOS")
    name = "macOS"

    buildType(_Self.buildTypes.CreateUniversalBuilds)

    buildType(arm64_Debug)
    buildType(arm64_Internal)
    buildType(arm64_TrinityDev)
    buildType(arm64_Release)

    buildType(x64_Debug)
    buildType(x64_Internal)
    buildType(x64_TrinityDev)
    buildType(x64_Release)
})

class CarbonBuildMacOS(buildName: String, configType: String, preset: String, agentArchitecture: String) : BuildType({
    id(buildName.toId())
    name = buildName
    maxRunningBuilds = 4
    artifactRules = "%env.CMAKE_INSTALL_PREFIX% => artifact.zip"

    params {
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
        param("env.GIT_TAG_HASH_OVERRIDE", "")
        param("env.CMAKE_CONFIG_TYPE", configType)
        param("env.CMAKE_GENERATOR", "Ninja Multi-Config")
        param("teamcity.vcsTrigger.runBuildInNewEmptyBranch", "true")
        param("github_checkout_folder", "github")
        param("env.CTEST_JUNIT_OUTPUT_FILE", "ctest_results.xml")
        select("env.VISUAL_STUDIO_PLATFORM_TOOLSET", "v141", label = "Visual Studio Platform Toolset", description = "Specify the toolset for the build. e.g. v141 or v143.",
                options = listOf("v141 (2017)" to "v141", "v143 (2022)" to "v143"))
        param("env.CMAKE_BUILD_TARGETS", "all")
        param("env.CMAKE_INSTALL_PREFIX", ".build-artifact")
        param("env.SENTRY_PROJECT", "exefile-crashes")
        param("env.CMAKE_BUILD_FOLDER", ".cmake-build-%build.number%")
        param("env.GIT_TAG_HASH", "")
        param("env.EXECUTABLE_FILENAMES_MATCH", "")
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
            name = "(macOS) Get Git Tag/Hash"
            workingDir = "%teamcity.build.checkoutDir%/%github_checkout_folder%"
            path = "python3"
            arguments = "%teamcity.build.checkoutDir%/carbon_pipeline_tools/carbon/GetGitTagAndOrHash.py"
        }
        exec {
            name = "(macOS) Remove build artifacts folder"
            path = "rm"
            arguments = "-rf %env.CMAKE_INSTALL_PREFIX%"
        }
        exec {
            name = "Configure"
            path = "cmake"
            arguments = "--preset %env.CMAKE_PRESET% -S %teamcity.build.checkoutDir%/%github_checkout_folder% -B %env.CMAKE_BUILD_FOLDER% -DINSTALL_TO_MONOLITH=ON -DCMAKE_INSTALL_PREFIX=%env.CMAKE_INSTALL_PREFIX% -DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root=%teamcity.build.checkoutDir%/%github_checkout_folder%/buildtrees -DBUILD_METAL=ON -DBUILD_SHADER_COMPILER=ON -DWITH_GRANNY=ON"
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
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X")
        startsWith("teamcity.agent.jvm.os.arch", agentArchitecture)
    }
})


