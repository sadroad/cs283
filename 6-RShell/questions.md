1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

Clients typically use length prefixes to know exactly how many bytes to expect, or delimiter markers that signal message completion. Alternative approaches include protocol-specific framing, chunked transfer encoding, or status flags that indicate when transmission is complete.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

Since TCP doesn't preserve message boundaries, protocols should implement framing mechanisms like length prefixes, unique delimiters, or structured message formats. Without proper boundary handling, commands may blend together, create synchronization issues, or cause clients to hang waiting for incomplete messages.

3. Describe the general differences between stateful and stateless protocols.

Stateful protocols maintain session information between requests, remembering client context and previous interactions. Stateless protocols treat each request as independent, requiring all necessary information in every message, making them more scalable but potentially less efficient for related operations.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

UDP offers lower latency, higher throughput, and reduced overhead compared to TCP, making it ideal for time-sensitive applications. It's perfect for real-time applications like gaming and VoIP, where occasional packet loss is acceptable but delays are not.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

Operating systems provide the Socket API, which abstracts network communications as file-like endpoints with functions for creating, connecting, sending, and receiving data. This standardized interface hides the complexity of hardware and protocol details while supporting different protocols and addressing schemes.
