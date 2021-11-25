import * as vscode from 'vscode';
import { Builder } from './builder';
import path = require("path");
import fs = require('fs');


class BuildMessageEmitter
{
	private emitter: vscode.EventEmitter<string>;

	constructor(emitter: vscode.EventEmitter<string>) {
		this.emitter = emitter;
	}

	status(text: string) {
		this.emitter.fire(this.convertNewLines(`\x1b[2m${text}\x1b[0m`));
	}

	message(text: string) {
		this.emitter.fire(this.convertNewLines(text));
	}

	private convertNewLines(text: string): string {
		return text.replace(/\n/g, '\r\n');
	}
}

function activate(context: vscode.ExtensionContext) {
	const builder = new Builder(vscode.workspace.workspaceFile ? vscode.workspace.workspaceFile.fsPath : '');
	 
	if (!builder.hasCompiler()) {
		vscode.window.showErrorMessage(`Could not find ShaderCompiler binary in ${builder.shaderCompilerPath}`);
		console.error(`CcpShaders extension could not find ShaderCompiler binary in ${builder.shaderCompilerPath}`);
	}
	else {
		console.log('CcpShaders is active. Using ShaderCompiler from ' + builder.shaderCompilerDir);
	}

	async function checkVersion() {
		const extension = vscode.extensions.getExtension('ccp.ccpshaders');
		if (!extension) {
			return;
		}
		const currentVersion = extension.packageJSON.version;
		const extensionDir = path.join(builder.shaderCompilerDir, 'vscode-extension', 'built');
		for (const fn of await fs.promises.readdir(extensionDir)) {
			let match = /ccpshaders-(.+)\.vsix/.exec(fn);
			if (match) {
				const version = match[1];
				if (currentVersion != version) {
					if (await vscode.window.showInformationMessage(`Branch contains a different version of CcpShaders extension (${version} while ${currentVersion} is installed). Would you like to install it?`, 'Install', 'Skip') == 'Install') {
						await vscode.commands.executeCommand('workbench.extensions.installExtension', vscode.Uri.file(path.join(extensionDir, fn)));
						vscode.commands.executeCommand('workbench.extensions.action.showInstalledExtensions');
						console.log(`installed ${fn}`);
						if (await vscode.window.showInformationMessage('You need to reload VS Code after installing the extension', 'Reload', 'Skip')) {
							vscode.commands.executeCommand("workbench.action.reloadWindow");
						}
					}
				}
			}
		}
	}

	checkVersion();


	function showFxInfo(shaderPath: vscode.Uri) {
		if (builder.osNameDir == 'macOS') {
			return builder.spawn('open', [path.join(builder.shaderCompilerDir, 'fxinfo', builder.osNameDir, 'fxinfo.app'), '--args', shaderPath.fsPath]);
		}
		else {
			return builder.spawn(path.join(builder.shaderCompilerDir, 'fxinfo', builder.osNameDir, 'fxinfo'), [shaderPath.fsPath]);
		}
	}

	function fsPaths(paths: Array<vscode.Uri>): Array<string> {
		return paths.map((x)=>x.fsPath);
	}

	function buildTask(roots: Array<vscode.Uri>, incremental: boolean): Promise<vscode.Pseudoterminal> {
		return new Promise(function (resolve, reject) {
			let writeEmitter = new vscode.EventEmitter<string>();
			let closeEmitter = new vscode.EventEmitter<void>();
			let canel = new vscode.CancellationTokenSource()
			resolve({
				onDidWrite: writeEmitter.event,
				onDidClose: closeEmitter.event,
				open: function () {
					builder.usePerforce = vscode.workspace.getConfiguration('ccp-shaders').get<boolean>('use-perforce', true);
					const config = vscode.workspace.getConfiguration('targetPlatforms');

					builder.enablePlatform('dx11', config.get<boolean>('directX11', true));
					builder.enablePlatform('dx12', config.get<boolean>('directX12', true));
					builder.enablePlatform('metal', config.get<boolean>('metal', true));

					let messageEmitter = new BuildMessageEmitter(writeEmitter);
					if (builder.hasDisabledPlatforms()) {
						messageEmitter.message('Warning: some target platforms were disabled in configuration settings\n');
					}
					builder.buildFiles(fsPaths(roots), incremental, messageEmitter, canel.token)
					.then(()=>{
						vscode.window.showInformationMessage(`Finished compiling shaders`);
					})
					.catch((err)=>{
						console.error(err);
						if (canel.token.isCancellationRequested) {
							return;
						}
						vscode.window.showWarningMessage('Encountered errors when compiling shaders');
					})
					.finally(()=>{
						closeEmitter.fire();
					});
				},
				close: function () {
					canel.cancel();
					canel.dispose();
				},
				setDimensions: function () {
				},
				handleInput: function () {
				}
			});
		});
	}

	context.subscriptions.push(vscode.commands.registerCommand('ccpshaders.compile', function (itemPath) {
		if (!itemPath) {
			if (vscode.window.activeTextEditor) {
				itemPath = vscode.window.activeTextEditor.document.uri;
			}
		}
		if (!itemPath) {
			return;
		}
		var execution = new vscode.CustomExecution(()=>buildTask([itemPath], false));
		var problemMatchers = ["$msCompile", "$metal"];
		vscode.tasks.executeTask(new vscode.Task({type: 'shadercompiler'}, vscode.TaskScope.Workspace, "compile", "ccpshaders", execution, problemMatchers));
	}));

	function _constructBuildExecution(incremental: boolean): vscode.CustomExecution {
		let roots: Array<vscode.Uri> = [];
		if (vscode.workspace.workspaceFolders) {
			for (let r of vscode.workspace.workspaceFolders) {
				roots.push(r.uri);
			}
		}
		return new vscode.CustomExecution(()=>buildTask(roots, incremental));
	}

	context.subscriptions.push(vscode.tasks.registerTaskProvider('shadercompiler', {
		provideTasks: function () {
            return [
				new vscode.Task(
					{type: 'shadercompiler'}, 
					vscode.TaskScope.Workspace, 
					"compile", 
					"ccpshaders", 
					_constructBuildExecution(true), 
					["$msCompile", "$metal"])
            ];
		},
		resolveTask: function (task: vscode.Task) {
			return new vscode.Task(
				task.definition, 
				vscode.TaskScope.Workspace, 
				"compile", 
				"ccpshaders", 
				_constructBuildExecution(task.definition.incremental), 
				["$msCompile", "$metal"]);
		}
	}));

	context.subscriptions.push(vscode.commands.registerCommand('ccpshaders.fxinfo', function (itemPath) {
		if (!itemPath) {
			if (vscode.window.activeTextEditor) {
				itemPath = vscode.window.activeTextEditor.document.uri;
			}
		}
		if (!itemPath) {
			return;
		}
		if (path.extname(itemPath.path).toLowerCase() != '.fx') {
			return;
		}
		showFxInfo(itemPath);
	}));
}
exports.activate = activate;

// this method is called when your extension is deactivated
function deactivate() {}

module.exports = {
	activate,
	deactivate
}
