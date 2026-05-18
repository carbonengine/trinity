package _Self.buildTypes

import jetbrains.buildServer.configs.kotlin.*
import jetbrains.buildServer.configs.kotlin.buildFeatures.vcsLabeling
import jetbrains.buildServer.configs.kotlin.triggers.vcs
import jetbrains.buildServer.configs.kotlin.buildSteps.python
import jetbrains.buildServer.configs.kotlin.buildSteps.script
import jetbrains.buildServer.configs.kotlin.buildSteps.exec

class UniversalBuild() : BuildType({
    name = "Create MacOS Universal Binaries"

    params{
        param("carbon-pipeline-tools-ref", "refs/heads/main")
        param("universal-output-dir", "%system.teamcity.build.workingDir%/output_build")
        param("universal-lib-path", "lib/macOS/universal/AppleClang/")
        param("universal-bin-path", "bin/macOS/universal/AppleClang/")
        param("x64-lib-path", "lib/macOS/x64")
        param("arm64-lib-path", "lib/macOS/arm64")
        param("x64-bin-path", "bin/macOS/x64")
        param("arm64-bin-path", "bin/macOS/arm64")
        text("file-matchers",
            "-m '*.so:%system.teamcity.build.workingDir%/lib' -m '*.dylib:%system.teamcity.build.workingDir%/lib' -m '*.a:%system.teamcity.build.workingDir%/lib'",
            label = "lipo-build matchers",
            description = """
            -m '*.so:lib' to tell the tool to lipo together .so files and output the fat binaries to lib/.
                            You can supply multiple matchers. You don't need to specify an output directory: -m '*.so'
                            Matchers without destination directories will be placed in 'universal-output-dir'
            """.trimIndent()
        )
    }

    artifactRules = "%universal-output-dir%"

    vcs {
        root(AbsoluteId("CarbonPipelineTools"), "+:carbon/.=>carbon")

        checkoutMode = CheckoutMode.ON_AGENT
        cleanCheckout = true
    }

    steps {
        exec {
            name = "Create output location"
            id = "output_location"
            path = "mkdir"
            arguments = "%universal-output-dir%"
        }
        script {
            name = "Populate output location"
            id = "populate_output_location"
            scriptContent = """
                cp -r %system.teamcity.build.workingDir%/x64/* %universal-output-dir%/
                cp -r %system.teamcity.build.workingDir%/arm64/* %universal-output-dir%/
            """.trimIndent()
        }
        python {
            name = "Run lipo"
            id = "run_lipo"
            environment = venv {
                requirementsFile = ""
            }
            command = file {
                filename = "carbon/lipo-builds.py"
                scriptArguments = "%file-matchers% -a %system.teamcity.build.workingDir%/arm64 -x %system.teamcity.build.workingDir%/x64 -o %system.teamcity.build.workingDir%"
            }
        }
        script {
            name = "Prepare universal build artifact"
            id = "prep_artifact"
            scriptContent = """
                mkdir -p   %universal-output-dir%/%universal-lib-path%
                mkdir -p   %universal-output-dir%/%universal-bin-path%
                cp %system.teamcity.build.workingDir%/lib/* %universal-output-dir%/%universal-lib-path%
                cp %system.teamcity.build.workingDir%/bin/* %universal-output-dir%/%universal-bin-path%
                rm -r %universal-output-dir%/%x64-lib-path%
                rm -r %universal-output-dir%/%arm64-lib-path%
            """.trimIndent()
        }
    }

    dependencies {
        dependency(MacOS.arm64_Debug) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/arm64"
            }
        }
        dependency(MacOS.x64_Debug) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/x64"
            }
        }

        dependency(MacOS.arm64_Release) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/arm64"
            }
        }
        dependency(MacOS.x64_Release) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/x64"
            }
        }

        dependency(MacOS.arm64_Internal) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/arm64"
            }
        }
        dependency(MacOS.x64_Internal) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/x64"
            }
        }

        dependency(MacOS.arm64_TrinityDev) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/arm64"
            }
        }
        dependency(MacOS.x64_TrinityDev) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = "**/*=>%system.teamcity.build.workingDir%/x64"
            }
        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X")
    }
})

val CreateUniversalBuilds = UniversalBuild()