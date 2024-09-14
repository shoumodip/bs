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

function copyClick(e) {
    const [, ok, a, b] = e.parentNode.children
    const code = a.classList.contains("active") ? a.innerText : b.innerText

    if (navigator.clipboard) {
        navigator.clipboard.writeText(code)
    } else {
        const textarea = document.createElement("textarea")
        textarea.setAttribute("style", "width: 1px; border:0; opacity:0;")
        document.body.appendChild(textarea)
        textarea.value = code
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
