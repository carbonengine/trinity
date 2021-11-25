import { CancellationToken, Disposable, Event } from "vscode";

import fs = require('fs');
import path = require("path");
import cp = require('child_process');
import os = require('os');
import process = require('process');
import yaml = require('js-yaml');

interface SpawnOptions
{
	input?: string,
	onStdOut?: (data: string)=>void,
	onStdErr?: (data: string)=>void,
	cancellationToken?: CancellationToken,
	env?: Record<string, string>
}

interface MessageEmitter
{
	status: (data: string)=>void;
	message: (data: string)=>void;
}

interface WorkItem
{
	file: string;
	platform: string;
	shaderModel: string;
	output: string;
	configs: number;
}

export class Builder
{
    shaderModels: Record<string, number> = {'lo': 3, 'hi': 4, 'depth': 5};
	platforms: Record<string, number> = {};
	supportedPlatforms: Record<string, number> = {};
    
    workspacePath: string;
    shaderCompilerDir: string;
	shaderCompilerPath: string;
	osNameDir: string;
	usePerforce: boolean;

    constructor (workspacePath: string) {
		this.workspacePath = workspacePath;
		this.usePerforce = true;
		this.shaderCompilerDir = path.join(path.dirname(workspacePath), '..', '..', '..', '..', '..', 'carbon', 'tools', 'ShaderCompiler');
		const platform = os.platform();
		switch (platform) {
		case 'win32':
			this.osNameDir = 'Windows';
			this.supportedPlatforms = {'dx11': 2, 'dx12': 6, 'metal': 10};
			break;
		case 'darwin':
			this.osNameDir = 'macOS';
			this.supportedPlatforms = {'metal': 10};
			break;
		default:
			this.osNameDir = platform;
		}

		for (const p in this.supportedPlatforms) {
			this.platforms[p] = this.supportedPlatforms[p];
		}
		this.shaderCompilerPath = path.join(this.shaderCompilerDir, this.osNameDir, 'ShaderCompiler');
	}
	
	enablePlatform(name: string, enable: boolean) {
		if (name in this.supportedPlatforms) {
			if (enable) {
				this.platforms[name] = this.supportedPlatforms[name];
			}
			else {
				delete this.platforms[name];
			}
		}
	}

	hasDisabledPlatforms(): boolean {
		for (const name in this.supportedPlatforms) {
			if (!(name in this.platforms)) {
				return true;
			}
		}
		return false;
	}

    hasCompiler(): boolean {
		if (os.platform() == 'win32') {
			return fs.existsSync(this.shaderCompilerPath + '.exe');
		}
		else {
			return fs.existsSync(this.shaderCompilerPath);
		}
    }

    spawn(exePath: string, args: Array<string>, options?: SpawnOptions): Promise<{stdout: string, stderr: string}> {
		let workspacePath = this.workspacePath;
        return new Promise(function (resolve, reject) {
			let rejected = false;
			let spawnOptions: Record<string, any> = {cwd: path.dirname(workspacePath)};
			if (options && options.env) {
				spawnOptions.env = {};
				for (let k in process.env) {
					spawnOptions.env[k] = process.env[k];
				}
				for (let k in options.env) {
					spawnOptions.env[k] = options.env[k];
				}
			}
            let proc = cp.spawn(exePath, args, spawnOptions);
            let disposable: Disposable | undefined = undefined;
            if (options && options.cancellationToken) {
                disposable = options.cancellationToken.onCancellationRequested(()=>proc.kill());
            }
            let stdOut = '';
            let stdErr = '';
            proc.stdout.on('data', function(data) {
                if (options && options.onStdOut) {
                    options.onStdOut(data.toString());
                }
                stdOut += data.toString();
            });
            proc.stderr.on('data', function(data) {
                if (options && options.onStdErr) {
                    options.onStdErr(data.toString());
                }
                stdErr += data.toString();
            });
            proc.on('exit', function (code) {
                if (disposable) {
                    disposable.dispose();
                }
                if (code == 0) {
                    resolve({stdout: stdOut, stderr: stdErr});
                }
                else if (!rejected) {
                    rejected = true;
                    reject(Error(`${exePath} returned an error code ${code}, stderr: ${stdErr}`));
                }
            });
            proc.on('error', function (err) {
                if (disposable) {
                    disposable.dispose();
                }
                if (!rejected) {
                    rejected = true;
                    reject(err);
                }
            });
            if (options && options.input) {
                proc.stdin.write(options.input);
                proc.stdin.end();
            }
        });
    }
    
	perforce(cmd: string, args?: Array<string>, token?: CancellationToken) {
		if (!this.usePerforce) {
			return;
		}
		let env: Record<string, string> = {'PWD': path.dirname(this.workspacePath)};
		if (!process.env.hasOwnProperty('P4CONFIG')) {
			env.P4CONFIG = 'p4config.txt';
		}
		return this.spawn('p4', ['-x', '-', cmd], {input: (args || []).join('\n'), env: env, cancellationToken: token});
	}

	private async * walk(dir: string): AsyncGenerator<string> {
		for await (const d of await fs.promises.readdir(dir)) {
			const entry = path.join(dir, d);
			const stat = await fs.promises.stat(entry);
			if (stat.isDirectory()) { 
				yield* this.walk(entry);
			}
			else if (stat.isFile() && path.extname(d).toLowerCase() == '.fx') {
				yield entry;
			}
		}
	}

	async flattenFiles(files: Array<string>, token?: CancellationToken): Promise<Array<string>> {
		let result = [];
		for (const f of files) {
			if (token && token.isCancellationRequested) {
				return [];
			}
			if ((await fs.promises.stat(f)).isDirectory()) {
				for await (const p of this.walk(f)) {
					if (token && token.isCancellationRequested) {
						return [];
					}
					result.push(p);
				}
			}
			else {
				result.push(f);
			}
		}
		return result;
	}

	getBuiltShaderPath(effectPath: string, platform: string, shaderModel: string) {
		effectPath = effectPath.replace(/\beffect\b/i, 'effect.' + platform);
		return effectPath.substr(0, effectPath.length - 2) + 'sm_' + shaderModel;
	}

	expandOutputs(sourcePaths: Array<string>, callback: (outPath: string, sourcePath: string, platform: string, shaderModel: string)=>void) {
		for (let f of sourcePaths) {
			for (let platform in this.platforms) {
				for (let sm in this.shaderModels) {
					let outPath = this.getBuiltShaderPath(f, platform, sm);
					callback(outPath, f, platform, sm);
				}
			}
		}
	}

	async getModifiedFiles(files: Array<string>, token?: CancellationToken) {
		if (files.length == 0) {
			return files;
		}
		const self = this;
		let sources: Record<string, string> = {};
		let lines: Array<string> = [];
		this.expandOutputs(files, function (outPath, sourcePath, platform, sm) {
			let line = `${sourcePath} ${outPath} SHADERMODEL ${self.shaderModels[sm]} PLATFORM ${self.platforms[platform]}`;
			sources[outPath] = sourcePath;
			lines.push(line);
		});

		let mtimeOutput = await this.spawn(this.shaderCompilerPath, ['/mtime'], {input: lines.join('\n'), cancellationToken: token});
		let modified = [];
		for (let line of mtimeOutput.stdout.split('\n')) {
			line = line.trim();
			if (!line.length) {
				continue;
			}
			let src = sources[line];
			if (!src) {
				continue;
			}
			if (modified.indexOf(src) < 0) {
				modified.push(src);
			}
		}
		return modified;
	}

	private async getShaderConfigCount(sourcePath: string) {
		let configs = (await this.spawn(this.shaderCompilerPath, ['/single', '/permutations', sourcePath])).stdout;
		if (configs.length == 0) {
			return 1;
		}
		let desc = yaml.load(configs);
		let count = 1;
		for (let k in desc) {
			let optionCount = 0;
			for (let j in desc[k].options) {
				++optionCount;
			}
			count *= optionCount;
		}
		return count;
	}

	private async compileFiles(files: Array<string>, emitter: MessageEmitter, token?: CancellationToken) {
		let workItems: Array<WorkItem> = [];
		let hasErrors = false;
		this.expandOutputs(files, function (outPath, sourcePath, platform, sm) {
			workItems.push({file: sourcePath, platform: platform, shaderModel: sm, output: outPath, configs: 0});
		});

		const maxThreads = os.cpus().length;

		let threads = 0;
		let self = this;

		async function pickWorkItem() {
			for (let i = 0; i < workItems.length; ++i) {
				if (workItems[i].configs == 0) {
					workItems[i].configs = await self.getShaderConfigCount(workItems[i].file);
				}
				if (threads == 0 || threads + workItems[i].configs <= maxThreads) {
					return workItems.splice(i, 1)[0];
				}
			}
			return undefined;
		}

		interface CurrentItem {
			promise: Promise<any> | undefined,
			item: WorkItem,
			hasOutput: boolean,
			finished: boolean
		}
		
		let currentItems: Array<CurrentItem> = [];

		let fileStatus: Record<string, boolean> = {};
		for (let f of files) {
			fileStatus[f] = true;
		}

		while (workItems.length) {
			if (token && token.isCancellationRequested) {
				return;
			}
			let item = await pickWorkItem();
			if (item) {
				let rec: CurrentItem = {promise: undefined, item: item, hasOutput: false, finished: false};
				emitter.status(`Compiling ${item.file}, ${item.platform}, ${item.shaderModel}\n\r`);
				await fs.promises.mkdir(path.dirname(item.output).toLowerCase(), {recursive: true});
				let promise = this.spawn(
					this.shaderCompilerPath, 
					['/single', '/define', 'SHADERMODEL', this.shaderModels[item.shaderModel].toString(), '/define', 'PLATFORM', this.platforms[item.platform].toString(), '/O3', item.file, item.output.toLowerCase()], 
					{
						cancellationToken: token,
						onStdOut: function (data) {
							rec.hasOutput = true;
							emitter.message(data);
						},
						onStdErr: function (data) {
							rec.hasOutput = true;
							emitter.message(data);
						}
					}
				)
				.catch(()=>{
					fileStatus[rec.item.file] = false;
					rec.finished = true;
					if (!rec.hasOutput) {
						emitter.message(`${rec.item.file}: error: ShaderCompiler exited with non-0 return code without producing any output\n`);
					}
					hasErrors = true;
				})
				.finally(()=>{
					rec.finished = true;
				});
				rec.promise = promise;
				currentItems.push(rec);
				threads += Math.min(item.configs, maxThreads);
			}
			else {
				try {
					await Promise.race(currentItems.map((x)=>x.promise));
				} catch (e) {
					hasErrors = true;
				}
				for (let i = 0; i < currentItems.length;) {
					if (currentItems[i].finished) {
						threads -= Math.min(currentItems[i].item.configs, maxThreads);
						currentItems.splice(i, 1);
					}
					else {
						++i;
					}
				}
			}
		}
		try {
			await Promise.all(currentItems.map((x)=>x.promise));
		} catch (e) {
			hasErrors = true;
		}

		if (hasErrors) {
			if (files.length > 1) {
				let failed = 0;
				for (let f in fileStatus) {
					if (!fileStatus[f]) {
						++failed;
					}
				}
				emitter.status(`Failed to compile ${failed} of ${files.length} files`);
			}
			throw Error('compilation failed');
		}
	}

	async buildFiles(files: Array<string>, incremental: boolean, emitter: MessageEmitter, token: CancellationToken | undefined): Promise<void> {
		files = await this.flattenFiles(files, token);

		if (incremental) {
			emitter.status('Searching for modified files...\n\r');
			let modified = await this.getModifiedFiles(files, token);
			emitter.status(`Found ${modified.length} modified files\n\r`);
			console.log('Modified files: ', modified);
			return await this.buildFiles(modified, false, emitter, token);
		}

		let outputs: Array<string> = [];
		this.expandOutputs(files, function (outPath) {
			outputs.push(outPath);
		});

		try {
			await this.perforce('edit', outputs, token);
		} catch (e) {
			if (!token || !token.isCancellationRequested) {
				emitter.message(`Failed to check out output files in perforce ${e.toString()}`);
			}
			throw e;
		}
		await this.compileFiles(files, emitter, token);
		await this.perforce('add', ['-tbinary+m'].concat(outputs), token);
	}

}
