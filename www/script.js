
const keys_enabled = {};
document.addEventListener("keydown", (e) => {
	console.log(e.key);
	if (e.altKey || e.ctrlKey || e.metaKey) return true;
	const k = e.key.toUpperCase();
	if (keys_enabled[k]) {
		insert_letter(k);
	} else if (k === "ENTER") {
		validate_row();
	} else if (k === 'BACKSPACE') {
		remove_letter();
	}
});
const keys = document.querySelectorAll(".key");
for (key of keys) {
	if (key.innerHTML.length == 1) {
		keys_enabled[key.innerHTML] = true;
		key.id = key.innerHTML;
		key.addEventListener('click', (e) => {
			console.log(e.currentTarget.innerHTML);
			insert_letter(e.currentTarget.innerHTML);
		});
	} else if (key.innerHTML == 'ENTER') {
		key.addEventListener('click', (e) => {
			validate_row();
		});
	} else {
		key.addEventListener('click', (e) => {
			remove_letter();
		});
	}
}

async function validate_row() {
	let crow = null;
	const grid = document.getElementById("grid");
	for (const row of grid.children) {
		if (!row.hasAttribute("revealed")) {
			let index = 0;		
			for (const cell of row.children) {
				if (cell.innerHTML.length === 0) {
					break;
				}
				index += 1;
			}
			if (index === 5) {
				crow = row;
				break;
			} else {
				break;
			}
		}
	}
	if (crow !== null) {
		crow.setAttribute("revealed", "");
		crow.outerHTML = crow.outerHTML; // removes eventListeners of children (https://stackoverflow.com/questions/4386300/how-to-remove-all-event-listeners-of-a-dom-object-in-javascript#:~:text=This%20will%20remove,element.outerHTML%3B)

		let pattern = "";
		let guess = "";
		for (const cell of crow.children) {
			const c = cell.innerHTML;
			guess += c;
			const key = document.getElementById(c);
			const color = cell.style.backgroundColor;
			const color_index = colors.findIndex((element) => element === color);
			
			if (color_index === 0) {
				pattern += "!";
			} else if (color_index === 1) {
				pattern += "?";
			} else if (color_index === 2) {
				pattern += "=";
			}

			const current_color_index = colors.findIndex((element) => element === key.style.backgroundColor);
			if (current_color_index === 0) {
				// skip
			} else if (current_color_index === 1) {
				if (color_index === 2) {
					key.style.backgroundColor = colors[2];
				}
			} else if (current_color_index === 2) {
				// skip
			} else {
				key.style.backgroundColor = colors[color_index];
				if (color_index == 0) {
					keys_enabled[c] = false;
					key.outerHTML = key.outerHTML;
				}
			}
		}
		// calculate
		const recommended = document.getElementById("recommended");
		recommended.innerHTML = '<div class="loader"></div>';
		
		const body = {
			pattern: pattern,
			word: guess.toLowerCase(),
		};

		const response = await fetch('/', {
			method: "POST",
			body: JSON.stringify(body),
		});
		if (response.status === 200) {
			const recommended_text = await response.text();
			recommended.innerHTML = recommended_text;
		} else {
			const error_text = await response.text();
			recommended.innerHTML = `<div style="color: red">${error_text}<div>`;
		}
	}
}


const colors = ["rgb(58, 58, 60)", "rgb(181, 159, 59)", "rgb(83, 141, 78)"];

function insert_letter(c) {
	if (typeof(c) !== "string") return false;
	if (c.length !== 1) return false;
	
	let ccell = null;
	let acell = null;
	const grid = document.getElementById("grid");
	let row_index = 0;
	for (const row of grid.children) {
		if (!row.hasAttribute("revealed")) {
			let index = 0;
			for (const cell of row.children) {
				if (cell.innerHTML.length === 0) {
					break;
				}
				index += 1;
			}
			if (index !== 5) {
				ccell = row.children[index];
				if (row_index > 0) {
					acell = grid.children[row_index - 1].children[index];
				}
			}
			break;
		}
		row_index += 1;
	}
	if (ccell === null) {
		console.log("No empty cell found.");
		return false;
	}
	ccell.innerHTML = c;
	if (acell !== null && acell.style.backgroundColor === colors[2] && acell.innerHTML === ccell.innerHTML) {
		ccell.style.backgroundColor = colors[2];
		ccell.style.borderColor = colors[2];
	} else {
		ccell.style.backgroundColor = colors[0];
		ccell.style.borderColor = colors[0];
	}
	ccell.addEventListener('click', (e) => {
		const color = e.currentTarget.style.backgroundColor;
		let index = colors.findIndex((element) => element === color);
		if (index === -1) {
			console.log(`color '${color}' not found`);
		} else {
			index = (index - 1 + colors.length) % colors.length;
			e.currentTarget.style.backgroundColor = colors[index];
			e.currentTarget.style.borderColor = colors[index];
		}
		return false;
	});
	ccell.addEventListener('contextmenu', (e) => {
		e.preventDefault();
		const color = e.currentTarget.style.backgroundColor;
		let index = colors.findIndex((element) => element === color);
		if (index === -1) {
			console.log(`color '${color}' not found`);
		} else {
			index = (index + 1) % colors.length;
			e.currentTarget.style.backgroundColor = colors[index];
			e.currentTarget.style.borderColor = colors[index];
		}
		return false;
	});
}

function remove_letter() {
	let ccell = null;
	const grid = document.getElementById("grid");
	for (const row of grid.children) {
		if (!row.hasAttribute("revealed")) {
			let index = 0;
			for (const cell of row.children) {
				if (cell.innerHTML.length === 0) {
					break;
				}
				index += 1;
			}
			index -= 1;
			if (index >= 0) {
				ccell = row.children[index];
			}
			break;
		}
	}
	if (ccell != null) {
		ccell.innerHTML = "";
		ccell.style.backgroundColor = "";
		ccell.style.borderColor = "#565758";
		ccell.outerHTML = ccell.outerHTML;
	}
}

///// CALCULATE CODE /////

/*const ALLOWED_ALL = 0xFFFFFFFF;
let State = {};
let Words = [];
let Infos = {};

function reset_words(word_infos) {
	State = {
		required: 0,
		allowed: ALLOWED_ALL,
		letters: [...Array(5)].map(x => { return {allowed: ALLOWED_ALL, found: ""}; }),
	};
	Words = [];
	Infos = {};
	for (let i = 0; i < word_infos.length; i++) {
		Words.push(word_infos[i].word);
		Infos[word_infos[i].word] = word_infos[i].info;
	}
}

function calculate_recommended(words, state, guess, pattern) {
	state_add_guess(state, guess, pattern);
	let new_words = [];
	for (let i = 0; i < words.length; i++) {
		if (word_fits_state(state, words[i])) {
			new_words.push(words[i]);
		}
	}
	console.log(`${words.length} -> ${new_words.length}`);
	let infos = {};
	for (let i = 0; i < new_words.length; i++) {
		console.log(`calucating info for word ${new_words[i]}`);
		let info = expected_info(state, new_words, guess);
		infos[new_words[i]] = info;
	}
	new_words.sort((a, b) => {
		return infos[a] - infos[b];
	});
	return { new_words, infos };
}

function state_disallow_char(state, c) {
	if (typeof(c) !== "string") return false;
	if (c.length !== 1) return false;
	state.alloweed &= ~(0x1 << (c.charCodeAt(0) - "a".charCodeAt(0)));
	return true;
}

function state_require_char(state, c) {
	if (typeof(c) !== "string") return false;
	if (c.length !== 1) return false;
	state.required |= 0x1 << (c.charCodeAt(0) - "a".charCodeAt(0));
	return true;
}

function letter_disallow_char(letter, c) {
	if (typeof(c) !== "string") return false;
	if (c.length !== 1) return false;
	letter.allowed &= ~(0x1 << (c.charCodeAt(0) - "a".charCodeAt(0)));
	return true;
}

function letter_found(letter, c) {
	if (typeof(c) !== "string") return false;
	if (c.length !== 1) return false;
	letter.found = c;
	return true;
}

const PATTERN_TYPE_GRAY = 0;
const PATTERN_TYPE_YELLOW = 1;
const PATTERN_TYPE_GREEN = 2;

function state_add_guess(state, guess, pattern) {
	if (typeof(pattern) !== "string") return false;
	if (pattern.length !== 5) return false;

	if (typeof(guess) !== "string") return false;
	if (guess.length !== 5) return false;

	for (let i = 0; i < 5; i++) {
		switch (pattern[i]) {
			case PATTERN_TYPE_GRAY:
			{
				state_disallow_char(state, guess[i]);
				break;
			}
			case PATTERN_TYPE_YELLOW:
			{
				letter_disallow_char(state.letters[i], guess[i]);
				state_require_char(state, guess[i]);
				break;
			}
			case PATTERN_TYPE_GREEN:
			{
				letter_found(state.letters[i], guess[i]);
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return true;
}

function word_fits_state(state, word) {
	if (typeof(word) !== "string") return false;
	if (word.length !== 5) return false;
	
	let a = "a".charCodeAt(0);

	let found_required = 0;
	for (let i = 0; i < 5; i++) {
		let c = word.charCodeAt(i);
		if (state.required & (0x1 << (c - a)) !== 0) {
			found_required |= 0x1 << (c - a);
		}
		if ((state.allowed & (0x1 << (c - a))) === 0) {
			return 0;
		}
		if (state.letters[i].found.length === 1 && state.letters[i].found !== c) {
			return 0;
		}
		if ((state.letters[i].allowed & (0x1 << (c - a))) === 0) {
			return 0;
		}
	}
	return state.required === found_required;
}

function count_words(state, words) {
	let count = 0;
	for (let i = 0; i < words.length; i++) {
		if (word_fits_state(state, words[i])) {
			count++;
		}
	}
	return count;
}


function expected_info(cstate, words, guess) {
	let info = 0.0;
	let pattern = [0, 0, 0, 0, 0];
	for (let a = 0; a < 3; a++) {
		pattern[0] = a;
		for (let b = 0; b < 3; b++) {
			pattern[1] = b;
			for (let c = 0; c < 3; c++) {
				pattern[2] = c;
				for (let d = 0; d < 3; d++) {
					pattern[3] = d;
					for (let e = 0; e < 3; e++) {
						pattern[4] = e;

						let state = structuredClone(cstate);
						state_add_guess(state, guess, pattern);
						
						let count = count_words(state, words);
						if (count != 0) {
							let p = count / words.length;
							info += p * Math.log2(1 / p);
						}
					}
				}
			}
		}
	}
	return info;
}
*/

