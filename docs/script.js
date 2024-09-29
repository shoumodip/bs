function tabClick(e) {
    // Do nothing if already active
    if (e.classList.contains("active")) {
        return
    }

    // Switch the active tab
    {
        const [a, b] = e.parentNode.children
        a.classList.toggle("active")
        b.classList.toggle("active")
    }

    // Switch the active code
    {
        const [, , a, b] = e.parentNode.parentNode.children[1].children
        a.classList.toggle("active")
        b.classList.toggle("active")
    }
}

function copyClick(e, ok, text) {
    if (navigator.clipboard) {
        navigator.clipboard.writeText(text)
    } else {
        const textarea = document.createElement("textarea")
        textarea.setAttribute("style", "width: 1px; border:0; opacity:0;")
        document.body.appendChild(textarea)
        textarea.value = text
        textarea.select()
        document.execCommand("copy")
        document.body.removeChild(textarea)
    }

    const toggle = () => {
        e.classList.toggle("hidden")
        ok.classList.toggle("hidden")
    }

    toggle();
    setTimeout(toggle, 1000)
}

function copyClickCode(e) {
    const [, ok, a, b] = e.parentNode.children
    copyClick(e, ok, a.classList.contains("active") ? a.innerText : b.innerText)
}

function copyClickShell(e) {
    const [, ok, a] = e.parentNode.children
    copyClick(e, ok, a.innerText)
}
