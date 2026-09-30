// Copyright © 2026 CCP ehf.

package _Self.buildTypes

import jetbrains.buildServer.configs.kotlin.BuildType
import jetbrains.buildServer.configs.kotlin.CheckoutMode
import jetbrains.buildServer.configs.kotlin.DslContext
import jetbrains.buildServer.configs.kotlin.buildFeatures.sshAgent
import jetbrains.buildServer.configs.kotlin.buildSteps.script
import jetbrains.buildServer.configs.kotlin.triggers.vcs

object SyncToMirror : BuildType({
    name = "Sync to Mirror"

    enablePersonalBuilds = false
    maxRunningBuilds = 1

    params {
        param("github_mirror_repository", "ccpgames/carbon-trinity-mirror")
        param("teamcity.vcsTrigger.runBuildInNewEmptyBranch", "true")
    }

    vcs {
        root(DslContext.settingsRootId)
        checkoutMode = CheckoutMode.ON_AGENT
        cleanCheckout = true
    }

    steps {
        script {
            name = "Mirror branches and tags to GitHub"
            // TeamCity gives us only the tag that triggered the build, not a full repository
            // TeamCity delivery state is also not trustworthy or intented to be manipulated for these purposes
            // Rather than relying on that checkout state, fetch all branches and tags ourselves
            scriptContent = """
                set -euo pipefail

                source_url="$(git -C "%teamcity.build.checkoutDir%" remote get-url origin)"
                mirror_dir="%teamcity.build.workingDir%/github-mirror.git"

                rm -rf "${'$'}mirror_dir"
                git init --bare "${'$'}mirror_dir"
                git -C "${'$'}mirror_dir" fetch --prune "${'$'}source_url" \
                    '+refs/heads/*:refs/heads/*' \
                    '+refs/tags/*:refs/tags/*'
                git -C "${'$'}mirror_dir" remote add destination \
                    "git@github.com:%github_mirror_repository%.git"
                git -C "${'$'}mirror_dir" push --mirror destination
            """.trimIndent()
        }
    }

    triggers {
        vcs {
            branchFilter = """
                +:*
                -:<default>
            """.trimIndent()
        }
    }

    features {
        sshAgent {
            teamcitySshKey = "ccpgames-carbon"
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Linux")
    }
})
